/*-------------------------------------------------------------------------------

	BARONY
	File: charclass.cpp
	Desc: character class definition code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "items.hpp"
#include "book.hpp"
#include "net.hpp"
#include "charclass.hpp"
#include "player.hpp"
#include "mod_tools.hpp"

void initializeWraithPlayerForNewGame(int player)
{
	if ( player < 0 || player >= MAXPLAYERS || !players[player] || !stats[player]
		|| stats[player]->playerRace != RACE_WRAITH
		|| stats[player]->stat_appearance != 0 )
	{
		return;
	}
	auto& wraith = players[player]->mechanics.wraith;
	if ( wraith.initialized )
	{
		return;
	}

	const Sint32 oldHP = stats[player]->HP;
	const Sint32 oldMaxHP = stats[player]->MAXHP;
	const Sint32 oldMP = stats[player]->MP;
	const Sint32 oldMaxMP = stats[player]->MAXMP;
	wraith.corporealMaxHP = oldMaxHP;
	wraith.corporealMaxMP = oldMaxMP;
	wraith.wasHumanPolymorphed = false;
	wraith.possessedEntityUid = 0;
	wraith.possessionTransition = false;
	wraith.possessedType = NOTHING;
	wraith.possessionStartPlayerLevel = 0;
	wraith.possessionLastPlayerLevel = 0;
	wraith.possessedBodyOriginalInvisible = false;
	wraith.possessedBodyOriginalPassable = false;
	wraith.possessedBodyOriginalBlocksSight = false;
	wraith.possessedBodyOriginalUnclickable = false;
	wraith.suspendedWraithEffects.fill(false);
	wraith.suspendedWraithEffectTimers.fill(0);
	wraith.temporaryInnateSpellIds.clear();
	wraith.possessionOriginalEquipmentUids.fill(0);
	wraith.learnedPossessionTypes.clear();
	wraith.seenBookLoreIds.clear();
	wraith.seenGravestoneLoreIds.clear();
	wraith.seenMailLoreIds.clear();
	wraith.readBookObjectIds.clear();
	wraith.readMailObjectIds.clear();
	wraith.readGravestoneObjectIds.clear();
	wraith.remoteLoreItems.clear();
	stats[player]->HP = 5;
	stats[player]->MAXHP = 5;
	stats[player]->MP = oldMP + std::max(0, oldHP - 5);
	stats[player]->MAXMP = oldMaxMP + std::max(0, oldMaxHP - 5);
	stats[player]->MAXMP = std::max<Sint32>(stats[player]->MAXMP,
		WRAITH_MIN_POSSESS_RESERVE);
	stats[player]->MP = std::max<Sint32>(stats[player]->MP,
		WRAITH_MIN_POSSESS_RESERVE);
	stats[player]->MP = std::min(stats[player]->MP, stats[player]->MAXMP);

	std::unordered_set<Item*> packed;
	std::vector<Item*> inventoryItems;
	for ( node_t* node = stats[player]->inventory.first; node; node = node->next )
	{
		if ( auto item = static_cast<Item*>(node->element) )
		{
			inventoryItems.push_back(item);
		}
	}
	for ( Item* item : inventoryItems )
	{
		if ( !item || item == stats[player]->ring || item == stats[player]->amulet
			|| item->type == SPELL_ITEM )
		{
			continue;
		}
		stats[player]->addItemToLootingBag(player, 0.0, 0.0, *item, false);
		packed.insert(item);
	}

	Item** equipment[] = {
		&stats[player]->helmet, &stats[player]->breastplate, &stats[player]->gloves,
		&stats[player]->shoes, &stats[player]->shield, &stats[player]->weapon,
		&stats[player]->cloak, &stats[player]->mask
	};
	for ( Item** slot : equipment )
	{
		Item* item = *slot;
		*slot = nullptr;
		if ( item && packed.find(item) == packed.end() )
		{
			stats[player]->addItemToLootingBag(player, 0.0, 0.0, *item, false);
			packed.insert(item);
		}
	}
	for ( Item* item : packed )
	{
		if ( item->node )
		{
			list_RemoveNode(item->node);
		}
		else
		{
			free(item);
		}
	}

	// The spirit has no free inventory slot for an ordinary consumable.
	// Place its starting MP potion inside the existing starting loot chest.
	Item* startingMagicPotion = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
	stats[player]->addItemToLootingBag(player, 0.0, 0.0, *startingMagicPotion, false);
	free(startingMagicPotion);

	const Uint32 bagKey = stats[player]->getLootingBagKey(player);
	Item* chest = newItem(TOOL_PLAYER_LOOT_BAG, WORN, 0, 1, bagKey, true,
		&stats[player]->inventory);
	wraith.initialized = true;

	if ( !setWraithHeldItem(player, chest) )
	{
		printlog(
			"[WRAITH]: ERROR failed to place starting loot chest in hand");
	}

	assert(stats[player]->weapon);
	assert(stats[player]->weapon->type == TOOL_PLAYER_LOOT_BAG);
	assert(itemSlot(stats[player], stats[player]->weapon)
		== &stats[player]->weapon);
	assert(itemIsEquipped(stats[player]->weapon, player));

	if ( players[player]->isLocalPlayer() )
	{
		assert(players[player]->paperDoll.getSlotForItem(
			*stats[player]->weapon) == Player::PaperDoll_t::SLOT_WEAPON);
	}
}


// Transmit the owning client's complete starting loot chest to the server.
// initClass() intentionally creates normal inventory items only for the local
// player. Without this reconciliation, the server's remote-Wraith starter
// chest lacks that class's locally generated consumables and equipment.
static Uint32 wraithStarterChestAckKey = 0;
static Uint32 wraithStarterChestAckRun = 0;

void acknowledgeWraithStartingChestSnapshot(int player, Uint32 key, Uint32 runKey)
{
    if ( multiplayer == CLIENT && player == clientnum )
    {
        wraithStarterChestAckKey = key;
        wraithStarterChestAckRun = runKey;
    }
}

void sendWraithStartingChestSnapshot(int player)
{
    if ( multiplayer != CLIENT || player != clientnum || !stats[player]
        || !players[player] || stats[player]->playerRace != RACE_WRAITH
        || !players[player]->mechanics.wraith.initialized || currentlevel != 0
        || !players[player]->entity
        || players[player]->entity->ticks >= 30 * TICKS_PER_SECOND
        || (players[player]->entity->ticks != 1
            && players[player]->entity->ticks % TICKS_PER_SECOND != 0) )
    {
        return;
    }
    const Uint32 bagKey = stats[player]->getLootingBagKey(player);
    if ( wraithStarterChestAckKey == bagKey
        && wraithStarterChestAckRun == uniqueGameKey ) { return; }
    auto found = stats[player]->player_lootbags.find(bagKey);
    if ( found == stats[player]->player_lootbags.end() || found->second.looted )
    {
        return;
    }
    const auto& contents = found->second.items;
    if ( contents.empty() || contents.size() > 128 ) { return; }
    const int customOffset = 62 + FAMILIAR_PERSISTENT_PACKET_BYTES;
    const int packetLength = customOffset + WRAITH_LORE_INSTANCE_PACKET_BYTES + 4;
    for ( size_t i = 0; i < contents.size(); ++i )
    {
        const Item& item = contents[i];
        strcpy((char*)net_packet->data, "WBGI");
        net_packet->data[4] = static_cast<Uint8>(player);
        SDLNet_Write32(bagKey, &net_packet->data[5]);
        net_packet->data[9] = static_cast<Uint8>(contents.size());
        net_packet->data[10] = static_cast<Uint8>(i);
        SDLNet_Write32(static_cast<Uint32>(item.type), &net_packet->data[11]);
        SDLNet_Write32(static_cast<Uint32>(item.status), &net_packet->data[15]);
        SDLNet_Write32(static_cast<Uint32>(item.beatitude), &net_packet->data[19]);
        SDLNet_Write32(static_cast<Uint32>(item.count), &net_packet->data[23]);
        SDLNet_Write32(item.appearance, &net_packet->data[27]);
        net_packet->data[31] = item.identified;
        net_packet->data[32] = static_cast<Uint8>(item.appliedOil);
        SDLNet_Write32(static_cast<Uint32>(item.runeGetStoredPWRRaw()), &net_packet->data[33]);
        net_packet->data[37] = static_cast<Uint8>(item.runeGetCreatorPlayer());
        SDLNet_Write32(item.runeGetInstanceId(), &net_packet->data[38]);
        SDLNet_Write32(item.runeGetCreatorIdentity(), &net_packet->data[42]);
        writeIngredientCountsToPacket(item, net_packet->data, 46);
        writeFamiliarPersistentStateToPacket(item, net_packet->data, 62);
        writeWraithLoreInstanceIdToPacket(item, net_packet->data, customOffset);
        SDLNet_Write32(uniqueGameKey, &net_packet->data[packetLength - 4]);
        net_packet->address.host = net_server.host;
        net_packet->address.port = net_server.port;
        net_packet->len = packetLength;
        sendPacketSafe(net_sock, -1, net_packet, 0);
    }
}



// A remote Wraith inherits the possessed creature's LOCAL client inventory.
// The vanilla host only mirrors some equipped items. Send a bounded, complete
// inventory view on changes and periodically. The host uses it ONLY to fill
// missing items when this particular possessed body is released/dies, never
// to overwrite live authoritative inventory.
void sendWraithBodyInventorySnapshot(int player, bool force)
{
    if ( multiplayer != CLIENT || player != clientnum || !players[player]
        || !players[player]->entity || !stats[player]
        || stats[player]->playerRace != RACE_WRAITH
        || !players[player]->mechanics.wraith.initialized )
    {
        return;
    }
    const Uint32 bodyUid = players[player]->mechanics.wraith.possessedEntityUid;
    if ( !bodyUid || bodyUid == UINT32_MAX ) { return; }
    const auto* spirit = stats[player];
    std::vector<const Item*> inventory;
    Uint32 digest = 2166136261u;
    const auto mix = [&digest](Uint32 value) { digest = (digest ^ value) * 16777619u; };
    for ( node_t* node = spirit->inventory.first; node; node = node->next )
    {
        const Item* item = static_cast<const Item*>(node->element);
        if ( !item || item->type == SPELL_ITEM || item->type == TOOL_PLAYER_LOOT_BAG
            || item == spirit->ring || item == spirit->amulet ) { continue; }
        if ( inventory.size() >= 128 ) { return; } // no truncated snapshots
        inventory.push_back(item);
        mix(static_cast<Uint32>(item->type));
        mix(item->appearance);
        mix(static_cast<Uint32>(item->status));
        mix(static_cast<Uint32>(item->beatitude));
        mix(static_cast<Uint32>(item->count));
        mix(static_cast<Uint32>(item->appliedOil));
        mix(static_cast<Uint32>(item->identified));
        mix(item->runeGetInstanceId());
        mix(item->runeGetCreatorIdentity());
        mix(item->familiarInstanceId);
        mix(item->wraithLoreInstanceId);
        for ( int slot = 0; slot < 4; ++slot )
        {
            mix(item->ingredientCounts[slot]);
        }
    }
    static Uint32 lastUid = 0, lastDigest = 0, lastSentTick = 0, lastRun = 0;
    if ( !force && lastUid == bodyUid && lastRun == uniqueGameKey && lastDigest == digest
        && static_cast<Uint32>(ticks - lastSentTick) < 2 * TICKS_PER_SECOND )
    {
        return;
    }
    lastUid = bodyUid;
    lastRun = uniqueGameKey;
    lastDigest = digest;
    lastSentTick = ticks;
    // A forced pre-release snapshot may be sent in the same game tick as a
    // regular snapshot. IDs must still increase, or the host will reject the
    // newer, complete snapshot as an already-processed sequence.
    static Uint32 lastSnapshotId = 0;
    const Uint32 snapshotId = std::max(ticks, lastSnapshotId + 1);
    lastSnapshotId = snapshotId;
    const int customOffset = 66 + FAMILIAR_PERSISTENT_PACKET_BYTES;
    const int packetLength = customOffset + WRAITH_LORE_INSTANCE_PACKET_BYTES + 4;
    // Send a well-defined empty snapshot to invalidate an old inventory view.
    const size_t frames = std::max<size_t>(1, inventory.size());
    for ( size_t i = 0; i < frames; ++i )
    {
        strcpy((char*)net_packet->data, "WBMI");
        net_packet->data[4] = static_cast<Uint8>(player);
        SDLNet_Write32(bodyUid, &net_packet->data[5]);
        SDLNet_Write32(snapshotId, &net_packet->data[9]);
        net_packet->data[13] = static_cast<Uint8>(inventory.size());
        net_packet->data[14] = static_cast<Uint8>(i);
        if ( !inventory.empty() )
        {
            const Item& item = *inventory[i];
            SDLNet_Write32(static_cast<Uint32>(item.type), &net_packet->data[15]);
            SDLNet_Write32(static_cast<Uint32>(item.status), &net_packet->data[19]);
            SDLNet_Write32(static_cast<Uint32>(item.beatitude), &net_packet->data[23]);
            SDLNet_Write32(static_cast<Uint32>(item.count), &net_packet->data[27]);
            SDLNet_Write32(item.appearance, &net_packet->data[31]);
            net_packet->data[35] = item.identified;
            net_packet->data[36] = static_cast<Uint8>(item.appliedOil);
            SDLNet_Write32(static_cast<Uint32>(item.runeGetStoredPWRRaw()), &net_packet->data[37]);
            net_packet->data[41] = static_cast<Uint8>(item.runeGetCreatorPlayer());
            SDLNet_Write32(item.runeGetInstanceId(), &net_packet->data[42]);
            SDLNet_Write32(item.runeGetCreatorIdentity(), &net_packet->data[46]);
            writeIngredientCountsToPacket(item, net_packet->data, 50);
            writeFamiliarPersistentStateToPacket(item, net_packet->data, 66);
            writeWraithLoreInstanceIdToPacket(item, net_packet->data, customOffset);
        }
        else
        {
            memset(&net_packet->data[15], 0, packetLength - 15);
        }
        SDLNet_Write32(uniqueGameKey, &net_packet->data[packetLength - 4]);
        net_packet->address.host = net_server.host;
        net_packet->address.port = net_server.port;
        net_packet->len = packetLength;
        sendPacketSafe(net_sock, -1, net_packet, 0);
    }
}


/*-------------------------------------------------------------------------------

	initClass

	Sets up a character class for the given player

-------------------------------------------------------------------------------*/

void initClassStats(const int classnum, void* myStats)
{
	if ( !myStats ) { return; }
	Stat* stat = static_cast<Stat*>(myStats);
	// CLASS LOADOUTS
	// barbarian
	if ( classnum == CLASS_BARBARIAN )
	{
		// attributes
		stat->STR += 2;
		stat->CON += 1;
		stat->DEX -= 0;
		stat->INT -= 2;
		stat->CHR -= 1;

		stat->MAXHP += 10;
		stat->HP += 10;
		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		//stat->setProficiency(PRO_LEGACY_SWIMMING, 25);
		stat->setProficiency(PRO_SHIELD, 25);
		stat->setProficiency(PRO_AXE, 50);
		stat->setProficiency(PRO_MACE, 25);
		stat->setProficiency(PRO_UNARMED, 20);
		stat->setProficiency(PRO_ALCHEMY, 10);
	}
	// warrior
	else if ( classnum == CLASS_WARRIOR )
	{
		// attributes
		stat->STR += 1;
		stat->DEX += 1;
		stat->CON -= 0;
		stat->INT -= 2;
		stat->PER -= 1;
		stat->CHR += 1;

		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_LEADERSHIP, 40);
		stat->setProficiency(PRO_RANGED, 25);
		stat->setProficiency(PRO_SWORD, 25);
		stat->setProficiency(PRO_POLEARM, 50);
		stat->setProficiency(PRO_SHIELD, 25);
		stat->setProficiency(PRO_UNARMED, 10);
	}
	// healer
	else if ( classnum == CLASS_HEALER )
	{
		// attributes
		stat->CON += 2;
		stat->INT += 1;
		stat->STR -= 1;
		stat->DEX -= 1;

		stat->MAXHP -= 10;
		stat->HP -= 10;
		stat->MAXMP += 10;
		stat->MP += 10;

		// skills
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 50);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 25);
		//stat->setProficiency(PRO_LEGACY_SWIMMING, 25);
		stat->setProficiency(PRO_THAUMATURGY, 50);
		stat->setProficiency(PRO_MYSTICISM, 25);
		stat->setProficiency(PRO_POLEARM, 25);
		stat->setProficiency(PRO_ALCHEMY, 30);
		stat->setProficiency(PRO_APPRAISAL, 10);
		stat->setProficiency(PRO_SHIELD, 10);
	}
	// rogue
	else if ( classnum == CLASS_ROGUE )
	{
		// attributes
		stat->DEX += 2;
		stat->PER += 2;
		stat->INT -= 1;
		stat->STR -= 1;
		stat->CHR -= 1;

		stat->MAXHP -= 5;
		stat->HP -= 5;
		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_APPRAISAL, 25);
		stat->setProficiency(PRO_STEALTH, 50);
		stat->setProficiency(PRO_LOCKPICKING, 40);
		stat->setProficiency(PRO_RANGED, 25);
		stat->setProficiency(PRO_SWORD, 25);
		stat->setProficiency(PRO_ALCHEMY, 20);
	}
	// wanderer
	else if ( classnum == CLASS_WANDERER )
	{
		// attributes
		stat->DEX += 1;
		stat->CON += 1;
		stat->INT -= 1;
		stat->PER += 1;
		stat->CHR -= 1;

		stat->MAXHP += 10;
		stat->HP += 10;
		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_STEALTH, 25);
		//stat->setProficiency(PRO_LEGACY_SWIMMING, 50);
		stat->setProficiency(PRO_POLEARM, 25);
		stat->setProficiency(PRO_RANGED, 25);
		stat->setProficiency(PRO_TRADING, 25);
		stat->setProficiency(PRO_UNARMED, 10);
		stat->setProficiency(PRO_ALCHEMY, 30);
	}
	// cleric
	else if ( classnum == CLASS_CLERIC )
	{
		// attributes
		stat->PER += 1;
		stat->CON += 1;
		stat->DEX -= 1;
		stat->CHR -= 0;

		// skills
		stat->setProficiency(PRO_MACE, 25);
		//stat->setProficiency(PRO_LEGACY_SWIMMING, 25);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 25);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 25);
		stat->setProficiency(PRO_THAUMATURGY, 40);
		stat->setProficiency(PRO_LEADERSHIP, 20);
		stat->setProficiency(PRO_ALCHEMY, 20);
		stat->setProficiency(PRO_SHIELD, 10);
	}
	// merchant
	else if ( classnum == CLASS_MERCHANT )
	{
		// attributes
		stat->CHR += 1;
		stat->PER += 1;
		stat->DEX -= 1;
		stat->INT -= 0;
		stat->GOLD += 1000;

		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_AXE, 25);
		stat->setProficiency(PRO_LEADERSHIP, 20);
		stat->setProficiency(PRO_APPRAISAL, 50);
		stat->setProficiency(PRO_TRADING, 50);
		stat->setProficiency(PRO_ALCHEMY, 10);
		stat->setProficiency(PRO_LOCKPICKING, 10);
	}
	// wizard
	else if ( classnum == CLASS_WIZARD )
	{
		// attributes
		stat->INT += 3;
		stat->PER += 1;
		stat->DEX -= 1;
		stat->CHR -= 1;

		stat->MAXHP -= 10;
		stat->HP -= 10;
		stat->MAXMP += 20;
		stat->MP += 20;

		// skills
		stat->setProficiency(PRO_POLEARM, 25);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 50);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 50);
		stat->setProficiency(PRO_SORCERY, 50);
		stat->setProficiency(PRO_THAUMATURGY, 15);
		stat->setProficiency(PRO_ALCHEMY, 10);
		stat->setProficiency(PRO_APPRAISAL, 10);
	}
	// arcanist
	else if ( classnum == CLASS_ARCANIST )
	{
		// attributes
		stat->STR -= 1;
		stat->DEX += 1;
		stat->CON -= 1;
		stat->INT += 1;
		stat->PER += 1;
		stat->CHR -= 1;

		stat->MAXHP -= 5;
		stat->HP -= 5;
		stat->MAXMP += 10;
		stat->MP += 10;

		// skills
		//stat->setProficiency(PRO_LEGACY_MAGIC, 25);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 50);
		stat->setProficiency(PRO_SORCERY, 30);
		//stat->setProficiency(PRO_MYSTICISM, 20);
		stat->setProficiency(PRO_STEALTH, 25);
		stat->setProficiency(PRO_LOCKPICKING, 25);
		stat->setProficiency(PRO_RANGED, 25);
	}
	// joker
	else if ( classnum == CLASS_JOKER )
	{
		// attributes
		stat->INT += 1;
		stat->CHR += 1;
		stat->CON -= 1;
		stat->STR -= 1;
		stat->GOLD += 200;

		// skills
		stat->setProficiency(PRO_LOCKPICKING, 25);
		stat->setProficiency(PRO_TRADING, 25);
		stat->setProficiency(PRO_LEADERSHIP, 20);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 25);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 25);
		stat->setProficiency(PRO_MYSTICISM, 25);
		stat->setProficiency(PRO_SORCERY, 10);
		stat->setProficiency(PRO_ALCHEMY, 10);
		stat->setProficiency(PRO_RANGED, 20);
		stat->setProficiency(PRO_STEALTH, 10);
	}
	// sexton
	else if ( classnum == CLASS_SEXTON )
	{
		// attributes
		stat->STR -= 1;
		stat->DEX += 1;
		stat->CON -= 1;
		stat->INT += 1;

		stat->MAXMP += 5;
		stat->MP += 5;

		// skills
		stat->setProficiency(PRO_MACE, 10);
		stat->setProficiency(PRO_SHIELD, 10);
		stat->setProficiency(PRO_STEALTH, 40);
		stat->setProficiency(PRO_MYSTICISM, 40);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 40);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 40);
		stat->setProficiency(PRO_RANGED, 20);
		stat->setProficiency(PRO_ALCHEMY, 20);
	}
	// ninja
	else if ( classnum == CLASS_NINJA )
	{
		// attributes
		stat->STR -= 0;
		stat->DEX += 2;
		stat->CON -= 1;
		stat->INT -= 2;
		stat->PER += 1;

		stat->MAXHP += 5;
		stat->HP += 5;

		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_STEALTH, 60);
		stat->setProficiency(PRO_SWORD, 60);
		stat->setProficiency(PRO_RANGED, 40);
	}
	// monk
	else if ( classnum == CLASS_MONK )
	{
		// attributes
		stat->STR += 1;
		stat->CON += 2;
		stat->PER -= 1;
		stat->CHR -= 0;

		stat->MAXHP += 10;
		stat->HP += 10;

		// skills
		stat->setProficiency(PRO_SHIELD, 40);
		stat->setProficiency(PRO_LEADERSHIP, 10);
		stat->setProficiency(PRO_THAUMATURGY, 25);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 10);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 20);
		stat->setProficiency(PRO_UNARMED, 50);
		stat->setProficiency(PRO_ALCHEMY, 20);
		//stat->setProficiency(PRO_LEGACY_SWIMMING, 10);
	}
	// start DLC
	else if ( classnum == CLASS_CONJURER )
	{
		// attributes
		stat->INT += 1;
		stat->CON += 2;
		stat->DEX -= 1;
		stat->PER -= 2;

		stat->MAXHP -= 0;
		stat->HP -= 0;
		stat->MAXMP += 15;
		stat->MP += 15;

		// skills
		//stat->setProficiency(PRO_LEGACY_MAGIC, 40);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 40);
		stat->setProficiency(PRO_MYSTICISM, 40);
		stat->setProficiency(PRO_THAUMATURGY, 15);
		stat->setProficiency(PRO_STEALTH, 20);
		stat->setProficiency(PRO_RANGED, 20);
		stat->setProficiency(PRO_LEADERSHIP, 40);
		stat->setProficiency(PRO_ALCHEMY, 20);
	}
	else if ( classnum == CLASS_ACCURSED )
	{
		// attributes
		stat->INT += 5;
		stat->STR -= 2;
		stat->CON -= 2;
		stat->DEX -= 3;
		stat->PER -= 1;

		stat->MAXHP -= 0;
		stat->HP -= 0;
		stat->MAXMP += 10;
		stat->MP += 10;

		// skills
		//stat->setProficiency(PRO_LEGACY_MAGIC, 70);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 40);
		stat->setProficiency(PRO_SORCERY, 60);
		stat->setProficiency(PRO_MYSTICISM, 20);
		stat->setProficiency(PRO_STEALTH, 40);
		stat->setProficiency(PRO_APPRAISAL, 20);
		stat->setProficiency(PRO_UNARMED, 40);
	}
	else if ( classnum == CLASS_MESMER )
	{
		// attributes
		stat->STR -= 2;
		stat->CON -= 3;
		stat->INT += 2;
		stat->DEX -= 2;
		stat->PER += 2;
		stat->CHR += 4;

		stat->MAXHP -= 5;
		stat->HP -= 5;
		stat->MAXMP += 10;
		stat->MP += 10;

		// skills
		stat->setProficiency(PRO_MYSTICISM, 60);
		stat->setProficiency(PRO_SORCERY, 25);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 60);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 40);
		stat->setProficiency(PRO_POLEARM, 20);
		stat->setProficiency(PRO_LEADERSHIP, 60);
	}
	else if ( classnum == CLASS_BREWER )
	{
		// attributes
		stat->STR += -2;
		stat->DEX += 1;
		stat->CON -= 2;
		stat->INT -= 2;
		stat->PER += 1;
		stat->CHR += 1;

		stat->MAXHP += 10;
		stat->HP += 10;
		stat->MAXMP -= 10;
		stat->MP -= 10;

		stat->GOLD = 100;

		// skills
		/*stat->setProficiency(PRO_MACE, 60);
		stat->setProficiency(PRO_SHIELD, 40);*/
		stat->setProficiency(PRO_AXE, 10);
		stat->setProficiency(PRO_UNARMED, 25);
		stat->setProficiency(PRO_TRADING, 10);
		stat->setProficiency(PRO_APPRAISAL, 10);
		stat->setProficiency(PRO_LEADERSHIP, 25);
		stat->setProficiency(PRO_ALCHEMY, 50);
	}
	else if ( classnum == CLASS_SHAMAN )
	{
		// attributes
		stat->STR -= 1;
		stat->INT += 2;
		stat->PER += 1;
		stat->CHR += 1;

		stat->MAXHP += 5;
		stat->HP += 5;

		stat->MAXMP += 10;
		stat->MP += 10;

		// skills
		stat->setProficiency(PRO_MYSTICISM, 40);
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 40);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 40);
		stat->setProficiency(PRO_UNARMED, 10);
		stat->setProficiency(PRO_ALCHEMY, 10);
		stat->setProficiency(PRO_STEALTH, 10);
		/*stat->setProficiency(PRO_SHIELD, 40);
		stat->setProficiency(PRO_LEADERSHIP, 10);
		stat->setProficiency(PRO_POLEARM, 10);
		stat->setProficiency(PRO_RANGED, 10);*/
	}
	else if ( classnum == CLASS_PUNISHER )
	{
		// attributes
		stat->STR -= 1;
		stat->DEX += 1;
		stat->CON -= 1;
		stat->INT -= 1;

		/*stat->MAXHP += 5;
		stat->HP += 5;

		stat->MAXMP += 10;
		stat->MP += 10;*/

		// skills
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 40);
		//stat->setProficiency(PRO_LEGACY_MAGIC, 20);
		stat->setProficiency(PRO_SORCERY, 20);
		stat->setProficiency(PRO_MYSTICISM, 20);
		stat->setProficiency(PRO_RANGED, 25);
		stat->setProficiency(PRO_AXE, 25);
		/*stat->setProficiency(PRO_SHIELD, 40);
		stat->setProficiency(PRO_LEADERSHIP, 10);
		stat->setProficiency(PRO_POLEARM, 10);
		stat->setProficiency(PRO_UNARMED, 50);
		stat->setProficiency(PRO_ALCHEMY, 20);*/
	}
	else if ( classnum == CLASS_HUNTER )
	{
		// attributes
		stat->STR -= 3;
		stat->DEX += 1;
		stat->PER += 3;
		stat->INT -= 3;
		stat->CON -= 1;

		stat->MAXHP -= 10;
		stat->HP -= 10;
		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		//stat->setProficiency(PRO_LEGACY_SPELLCASTING, 10);
		stat->setProficiency(PRO_APPRAISAL, 20);
		stat->setProficiency(PRO_STEALTH, 25);
		//stat->setProficiency(PRO_LEGACY_SWIMMING, 50);
		stat->setProficiency(PRO_RANGED, 50);
		stat->setProficiency(PRO_LOCKPICKING, 10);
	}
	else if ( classnum == CLASS_MACHINIST )
	{
		// attributes
		stat->STR -= 2;
		//stat->DEX -= 2;
		stat->CON -= 3;
		stat->INT += 1;
		stat->PER += 0;

		stat->MAXHP -= 5;
		stat->HP -= 5;

		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_LOCKPICKING, 40);
		stat->setProficiency(PRO_RANGED, 10);
		stat->setProficiency(PRO_ALCHEMY, 10);
		stat->setProficiency(PRO_TRADING, 10);
	}
	else if ( classnum == CLASS_BARD )
	{
		stat->STR += -2;
		stat->DEX += 1;
		stat->CON += -2;
		stat->INT += 0;
		stat->PER += 0;
		stat->CHR += 3;

		stat->MAXHP -= 5;
		stat->HP -= 5;

		stat->MAXMP += 5;
		stat->MP += 5;

		stat->GOLD = 200;

		stat->setProficiency(PRO_SWORD, 25);
		stat->setProficiency(PRO_APPRAISAL, 50);
		stat->setProficiency(PRO_MYSTICISM, 15);
		stat->setProficiency(PRO_LEADERSHIP, 15);
		stat->setProficiency(PRO_STEALTH, 15);
		stat->setProficiency(PRO_TRADING, 15);
	}
	else if ( classnum == CLASS_SAPPER )
	{
		stat->STR += 1;
		stat->DEX += 1;
		stat->CON += -3;
		stat->INT += -2;
		stat->PER += 0;
		stat->CHR += -3;

		//stat->MAXHP += 5;
		//stat->HP += 5;

		stat->MAXMP -= 5;
		stat->MP -= 5;

		stat->setProficiency(PRO_MACE, 50);
		stat->setProficiency(PRO_SORCERY, 15);
		stat->setProficiency(PRO_RANGED, 15);
		stat->setProficiency(PRO_STEALTH, 25);
		stat->setProficiency(PRO_ALCHEMY, 25);
	}
	else if ( classnum == CLASS_SCION )
	{
		// attributes
		stat->STR -= 2;
		stat->INT += 3;
		stat->PER += 1;
		stat->DEX -= 2;
		stat->CHR -= 1;

		stat->MAXHP -= 10;
		stat->HP -= 10;
		stat->MAXMP += 20;
		stat->MP += 20;
		stat->GOLD = 250;

		// skills
		stat->setProficiency(PRO_SORCERY, 40);
		stat->setProficiency(PRO_MYSTICISM, 40);
		stat->setProficiency(PRO_THAUMATURGY, 40);
		stat->setProficiency(PRO_ALCHEMY, 10);
		stat->setProficiency(PRO_APPRAISAL, 10);
	}
	else if ( classnum == CLASS_HERMIT )
	{
		// attributes
		stat->INT += 1;
		stat->CON += 2;
		stat->STR -= 2;
		stat->DEX -= 2;
		stat->CHR += 1;

		//stat->MAXHP -= 10;
		//stat->HP -= 10;
		//stat->MAXMP += 20;
		//stat->MP += 20;
		//stat->GOLD = 250;

		// skills
		stat->setProficiency(PRO_MYSTICISM, 40);
		stat->setProficiency(PRO_SORCERY, 15);
		stat->setProficiency(PRO_MACE, 25);
		stat->setProficiency(PRO_STEALTH, 25);
		stat->setProficiency(PRO_ALCHEMY, 10);
	}
	else if ( classnum == CLASS_PALADIN )
	{
		// attributes
		stat->CON += 0;
		stat->STR += 1;
		stat->DEX -= 1;
		stat->INT += 1;
		stat->PER -= 1;
		stat->CHR += 1;

		//stat->MAXHP -= 10;
		//stat->HP -= 10;
		stat->MAXMP -= 5;
		stat->MP -= 5;
		//stat->GOLD = 250;

		// skills
		stat->setProficiency(PRO_SWORD, 50);
		stat->setProficiency(PRO_THAUMATURGY, 25);
		stat->setProficiency(PRO_APPRAISAL, 25);
		stat->setProficiency(PRO_SHIELD, 10);
		stat->setProficiency(PRO_LEADERSHIP, 10);
	}
	// mod add: whaler 
	else if ( classnum == CLASS_WHALER )
	{
		// attributes
		stat->STR += 2;
		stat->DEX -= 2;
		stat->CON += 1;
		stat->INT -= 2;
		stat->PER += 1;

		stat->MAXHP += 5;
		stat->HP += 5;

		stat->MAXMP -= 10;
		stat->MP -= 10;

		// skills
		stat->setProficiency(PRO_STEALTH, 20);
		stat->setProficiency(PRO_POLEARM, 40);
		stat->setProficiency(PRO_RANGED, 50);
		stat->setProficiency(PRO_LOCKPICKING, 20);
	} //mod add end
	// mod add: Gunslinger, initially copied from the Tinkerer/Mechanist profile.
	else if ( classnum == CLASS_GUNSLINGER )
	{
		// attributes
		stat->STR -= 3;
		stat->DEX += 1;
		stat->CON += 0;
		stat->INT += 1;
		stat->PER += 1;

		stat->MAXHP -= 5;
		stat->HP -= 5;

		stat->MAXMP += 10;
		stat->MP += 10;

		// skills
		stat->setProficiency(PRO_LOCKPICKING, 40);
		stat->setProficiency(PRO_RANGED, 30);
		stat->setProficiency(PRO_SWORD, 20);
		stat->setProficiency(PRO_UNARMED, 20);
		stat->setProficiency(PRO_SORCERY, 10);
	} //mod add end
	//mod add runesmith
	else if ( classnum == CLASS_RUNESMITH )
	{
		// attributes
		stat->CON += 0;
		stat->STR += 1;
		stat->DEX -= 1;
		stat->INT += 1;
		stat->PER -= 1;
		stat->CHR += 0;

		stat->MAXHP -= 10;
		stat->HP -= 10;

		stat->MAXMP += 15;
		stat->MP += 15;

		// skills
		stat->setProficiency(PRO_SORCERY, 20);
		stat->setProficiency(PRO_MYSTICISM, 20);
		stat->setProficiency(PRO_THAUMATURGY, 20);
		stat->setProficiency(PRO_POLEARM, 15);
		stat->setProficiency(PRO_APPRAISAL, 30);
	} //mod add end
	// mod add: Witch
	else if ( classnum == CLASS_WITCH )
	{
		stat->STR -= 1;
		stat->DEX += 0;
		stat->CON += 0;
		stat->INT += 2;
		stat->PER += 1;
		stat->CHR += 1;

		stat->MAXHP += 5;
		stat->HP += 5;
		stat->MAXMP += 10;
		stat->MP += 10;

		stat->setProficiency(PRO_ALCHEMY, 40);
		stat->setProficiency(PRO_TRADING, 30);
		stat->setProficiency(PRO_APPRAISAL, 25);
	}
	if ( gameModeManager.currentSession.challengeRun.isActive() )
	{
		if ( gameModeManager.currentSession.challengeRun.customBaseStats )
		{
			auto& s = gameModeManager.currentSession.challengeRun.baseStats;
			stat->HP = s->HP;
			stat->MAXHP = s->MAXHP;
			stat->MP = s->MP;
			stat->MAXMP = s->MAXMP;

			stat->STR = s->STR;
			stat->DEX = s->DEX;
			stat->CON = s->CON;
			stat->INT = s->INT;
			stat->PER = s->PER;
			stat->CHR = s->CHR;

			stat->EXP = s->EXP;
			stat->LVL = s->LVL;
			stat->GOLD = s->GOLD;

			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				stat->setProficiency(i, s->getProficiency(i));
			}
		}
		if ( gameModeManager.currentSession.challengeRun.customAddStats )
		{
			auto& s = gameModeManager.currentSession.challengeRun.addStats;
			stat->HP += s->HP;
			stat->MAXHP += s->MAXHP;
			stat->MP += s->MP;
			stat->MAXMP += s->MAXMP;

			stat->STR += s->STR;
			stat->DEX += s->DEX;
			stat->CON += s->CON;
			stat->INT += s->INT;
			stat->PER += s->PER;
			stat->CHR += s->CHR;

			stat->EXP += s->EXP;
			stat->LVL += s->LVL;
			stat->GOLD += s->GOLD;

			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				stat->setProficiency(i, stat->getProficiency(i) + s->getProficiency(i));
			}
		}
	}
	stat->HP = std::max(1, stat->HP);
	stat->MAXHP = std::max(1, stat->MAXHP);
	stat->MP = std::max(1, stat->MP);
	stat->MAXMP = std::max(1, stat->MAXMP);
	stat->OLDHP = stat->HP;
}

static bool shouldInvertStartingEquipmentBeatitude(const Stat* playerStats)
{
	if ( !playerStats || playerStats->stat_appearance != 0 )
	{
		return false;
	}
	if ( playerStats->playerRace == RACE_SUCCUBUS
		|| playerStats->playerRace == RACE_INCUBUS )
	{
		return true;
	}
	return playerStats->playerRace == RACE_GOLEM && playerStats->sex == FEMALE;
}

void initClass(const int player)
{
	Item* item = nullptr;
	Item* item2 = nullptr;
	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();

	bool isLocalPlayer = players[player]->isLocalPlayer();

	if ( isLocalPlayer )
	{
		//TODO: Dedicated gameStartStuff() function. Seriously.
		//(same for deathStuff() and/or gameEndStuff().
		players[player]->inventoryUI.selectSlot(0, 0);
		players[player]->inventoryUI.selectSpell(0, 0);
		players[player]->inventoryUI.selectChestSlot(0, 0);
		hotbar_t.clear();
		players[player]->paperDoll.clear();
	}

	const bool curseItems = shouldInvertStartingEquipmentBeatitude(stats[player]);

	//stats[player]->STR += 1;

	stats[player]->type = HUMAN;

	//spellList = malloc(sizeof(list_t));
	players[player]->magic.spellList.first = nullptr;
	players[player]->magic.spellList.last = nullptr;


	// CLASS LOADOUTS
	// barbarian
	if ( client_classes[player] == CLASS_BARBARIAN )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}
        
        // ring of strength
        item = newItem(RING_STRENGTH, WORN, 0, 1, 0, true, nullptr);
        if ( isLocalPlayer )
        {
            item2 = itemPickup(player, item);
            useItem(item2, player);
            free(item);
        }
        else
        {
            useItem(item, player);
        }

		// iron axe
		item = newItem(IRON_AXE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// wooden shield
		item = newItem(WOODEN_SHIELD, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[4].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather helm
		item = newItem(LEATHER_HELM, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// meat
			item = newItem(FOOD_MEAT, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// torch
			item = newItem(TOOL_TORCH, WORN, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// booze
			item = newItem(POTION_BOOZE, EXCELLENT, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// tomahawk
			item = newItem(BRONZE_TOMAHAWK, DECREPIT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	// warrior
	else if ( client_classes[player] == CLASS_WARRIOR )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// iron spear
		item = newItem(IRON_SPEAR, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// bronze shield
		item = newItem(BRONZE_SHIELD, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather helm
		item = newItem(LEATHER_HELM, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// iron armor
		item = newItem(IRON_BREASTPIECE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// boots
		item = newItem(LEATHER_BOOTS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// fish
			item = newItem(FOOD_FISH, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// healer
	else if ( client_classes[player] == CLASS_HEALER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// quarterstaff
		item = newItem(QUARTERSTAFF, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// phrygian hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// healer doublet
		item = newItem(HEALER_DOUBLET, SERVICABLE, 0, 1, 2, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}
		
		// cloak (red, protection)
		item = newItem(CLOAK, SERVICABLE, 0, 1, 2, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}


		if ( isLocalPlayer )
		{
			// fish
			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// extra healing potions
			item = newItem(POTION_EXTRAHEALING, EXCELLENT, 0, 3, 6, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// cure ailment spellbook
			item = newItem(SPELLBOOK_CUREAILMENT, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[7].item = item2->uid;
			}
			else
			{
				hotbar[8].item = item2->uid;
			}
			free(item);

			// healing spellbook
			item = newItem(SPELLBOOK_HEALING, EXCELLENT, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[8].item = item2->uid;
			}
			else
			{
				hotbar[9].item = item2->uid;
			}
			free(item);

			// apples
			item = newItem(FOOD_APPLE, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// towels
			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// magicstaff of slow
			item = newItem(MAGICSTAFF_SLOW, SERVICABLE, 0, 1, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	// rogue
	else if ( client_classes[player] == CLASS_ROGUE )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// bronze sword
		item = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak (green)
		item = newItem(CLOAK, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// hood (green)
		item = newItem(HAT_HOOD_WHISPERS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// gloves
		item = newItem(GLOVES, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// sickness potion
			item = newItem(POTION_SICKNESS, EXCELLENT, 0, 5, 5, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// shortbow
			item = newItem(SHORTBOW, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// ammo
			item = newItem(QUIVER_KNOCKBACK, SERVICABLE, 0, 15, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// noisemaker
			item = newItem(TOOL_DECOY, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// lockpicks
			item = newItem(TOOL_LOCKPICK, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);
		}
	}
	// wanderer
	else if ( client_classes[player] == CLASS_WANDERER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// quarterstaff
		item = newItem(QUARTERSTAFF, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// brown hood
		item = newItem(HAT_HOOD_WHISPERS, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak
		item = newItem(CLOAK, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, curseItems ? -1 : 1, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// ring warning
		item = newItem(RING_WARNING, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// crossbow
			item = newItem(SLING, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// lantern
			item = newItem(TOOL_LANTERN, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// tins
			item = newItem(FOOD_TIN, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// fish
			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// tin opener
			item = newItem(TOOL_TINOPENER, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// towel
			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// scroll
			item = newItem(SCROLL_MAGICMAPPING, SERVICABLE, 0, 6, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// book
			item = newItem(SPELLBOOK_DETECT_FOOD, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// cure ailment
			item = newItem(POTION_CUREAILMENT, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);
		}
	}
	// cleric
	else if ( client_classes[player] == CLASS_CLERIC )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// iron mace
		item = newItem(IRON_MACE, SERVICABLE, curseItems ? -1 : 1, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// wooden shield
		item = newItem(WOODEN_SHIELD, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak (purple)
		item = newItem(CLOAK, SERVICABLE, 0, 1, 3, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// gloves
		item = newItem(BRACERS, WORN, curseItems ? -1 : 1, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// healing potions
			item = newItem(POTION_HEALING, EXCELLENT, 0, 2, 7, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// summon scrolls
			item = newItem(SCROLL_SUMMON, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(SPELLBOOK_TROLLS_BLOOD, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[8].item = item2->uid;
			}
			else
			{
				hotbar[9].item = item2->uid;
			}
			free(item);
		}
	}
	// merchant
	else if ( client_classes[player] == CLASS_MERCHANT )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// bronze axe
		item = newItem(BRONZE_AXE, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// phrygian hat
		item = newItem(HAT_PHRYGIAN, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// glasses
		item = newItem(MONOCLE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// pickaxe
			item = newItem(TOOL_PICKAXE, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// gloves
			item = newItem(GLOVES, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);

			// scroll of remove curse
			item = newItem(SCROLL_REMOVECURSE, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// scroll of identify
			item = newItem(SCROLL_IDENTIFY, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// cheese
			item = newItem(FOOD_CHEESE, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// meat
			item = newItem(FOOD_MEAT, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// wizard
	else if ( client_classes[player] == CLASS_WIZARD )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// quarterstaff
		item = newItem(QUARTERSTAFF, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// wizard hat
		item = newItem(HAT_WIZARD, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// wizard doublet
		item = newItem(WIZARD_DOUBLET, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak (purple, protection)
		item = newItem(CLOAK_PROTECTION, SERVICABLE, 0, 1, 3, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// amulet of magic reflection
		item = newItem(AMULET_MAGICREFLECTION, EXCELLENT, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// magicstaff of light
			item = newItem(MAGICSTAFF_LIGHT, EXCELLENT, 0, 1, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// potion of restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// spellbook of fireball
			item = newItem(SPELLBOOK_FIREBALL, SERVICABLE, 0, 1, 3, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[6].item = item2->uid;
			}
			else
			{
				hotbar[7].item = item2->uid;
			}
			free(item);

			// spellbook of cold
			item = newItem(SPELLBOOK_COLD, SERVICABLE, 0, 1, 4, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[7].item = item2->uid;
			}
			else
			{
				hotbar[8].item = item2->uid;
			}
			free(item);

			// spellbook of light
			item = newItem(SPELLBOOK_LIGHT, SERVICABLE, 0, 1, 5, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[8].item = item2->uid;
			}
			else
			{
				hotbar[9].item = item2->uid;
			}
			free(item);
		}
	}
	// arcanist
	else if ( client_classes[player] == CLASS_ARCANIST )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// crossbow
		item = newItem(CROSSBOW, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather armor
		item = newItem(LEATHER_BREASTPIECE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak (purple)
		item = newItem(CLOAK, WORN, 0, 1, 3, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// hood (purple)
		item = newItem(HAT_HOOD_APPRENTICE, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// ammo
			item = newItem(QUIVER_FIRE, SERVICABLE, 0, 15, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// magicstaff of opening
			item = newItem(MAGICSTAFF_OPENING, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// potion of restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// fire staff
			item = newItem(MAGICSTAFF_FIRE, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// spellbook of forcebolt
			item = newItem(SPELLBOOK_FORCEBOLT, WORN, 0, 1, 6, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[7].item = item2->uid;
			}
			else
			{
				hotbar[8].item = item2->uid;
			}
			free(item);

			// scroll
			item = newItem(SCROLL_CHARGING, EXCELLENT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// joker
	else if ( client_classes[player] == CLASS_JOKER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// jester hat
		item = newItem(HAT_JESTER, SERVICABLE, curseItems ? -1 : 1, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// slingshot
			item = newItem(SLING, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// lockpicks
			item = newItem(TOOL_LOCKPICK, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// scroll of teleportation
			item = newItem(SCROLL_TELEPORTATION, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// potion
			item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// scroll of food
			item = newItem(SCROLL_FOOD, EXCELLENT, 0, 1, 0, false, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// ring of levitation
			item = newItem(RING_LEVITATION, SERVICABLE, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// spellbook of confuse
			item = newItem(SPELLBOOK_CONFUSE, WORN, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[8].item = item2->uid;
			}
			else
			{
				hotbar[9].item = item2->uid;
			}
			free(item);

			// blindfold
			item = newItem(TOOL_BLINDFOLD, SERVICABLE, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// luckstone
			item = newItem(GEM_LUCK, EXCELLENT, 0, 1, 1, false, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// food
			item = newItem(FOOD_CREAMPIE, SERVICABLE, 0, 8, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);
		}
	}
	// sexton
	else if ( client_classes[player] == CLASS_SEXTON )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// bronze mace
		item = newItem(BRONZE_MACE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// shard
		item = newItem(TOOL_CRYSTALSHARD, SERVICABLE, 0, 2, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[2].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather breastpiece
		item = newItem(LEATHER_BREASTPIECE, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather boots
		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// purple hood
		item = newItem(HAT_FEZ, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// chakram
			item = newItem(STEEL_CHAKRAM, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_TOMALLEY, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// books
			item = newItem(SPELLBOOK_SLEEP, WORN, 0, 1, 7, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[6].item = item2->uid;
			}
			else
			{
				hotbar[7].item = item2->uid;
			}
			free(item);

			item = newItem(SPELLBOOK_OPENING, WORN, 0, 1, 6, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[7].item = item2->uid;
			}
			else
			{
				hotbar[8].item = item2->uid;
			}
			free(item);

			item = newItem(SPELLBOOK_LOCKING, WORN, 0, 1, 6, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[8].item = item2->uid;
			}
			else
			{
				hotbar[9].item = item2->uid;
			}
			free(item);
		}
	}
	// ninja
	else if ( client_classes[player] == CLASS_NINJA )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// katana
		item = newItem(CRYSTAL_SWORD, DECREPIT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// black hood
		item = newItem(HAT_HOOD_ASSASSIN, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// black mask
		item = newItem(MASK_BANDIT, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// tunic
		item = newItem(TUNIC, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather boots
		item = newItem(LEATHER_BOOTS_SPEED, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// black cloak
		item = newItem(CLOAK_BLACK, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// daggers
			item = newItem(IRON_DAGGER, WORN, 0, 5, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// bear trap
			item = newItem(TOOL_BEARTRAP, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
			
			// paralyze potion
			item = newItem(POTION_PARALYSIS, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			// invis potion
			item = newItem(POTION_INVISIBILITY, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// teleport scroll
			item = newItem(SCROLL_TELEPORTATION, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);
		}
	}
	// monk
	else if ( client_classes[player] == CLASS_MONK )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// knuckles
		item = newItem(BRASS_KNUCKLES, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// tunic
		item = newItem(TUNIC, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// ring slow digestion
		item = newItem(RING_SLOWDIGESTION, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// torch
			item = newItem(TOOL_TORCH, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// light book
			item = newItem(SPELLBOOK_LIGHT, WORN, 0, 1, 7, true, nullptr);
			item2 = itemPickup(player, item);
			if ( players[player]->hotbar.useHotbarFaceMenu )
			{
				hotbar[8].item = item2->uid;
			}
			else
			{
				hotbar[9].item = item2->uid;
			}
			free(item);
		}
	}
	// start DLC
	else if ( client_classes[player] == CLASS_CONJURER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// weapon
		item = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(TOOL_LANTERN, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// red hood
		item = newItem(HAT_HOOD_APPRENTICE, SERVICABLE, 0, 1, 2, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// red cloak
		item = newItem(CLOAK, SERVICABLE, 0, 1, 2, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// slow book
			item = newItem(SPELLBOOK_POISON, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_ACCURSED )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// doublet
		item = newItem(SILVER_DOUBLET, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// gloves
		item = newItem(SUEDE_GLOVES, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// boots
		item = newItem(SUEDE_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// invis book
			item = newItem(SPELLBOOK_INVISIBILITY, WORN, 0, 1, 2, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// blood
			item = newItem(FOOD_BLOOD, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_MESMER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// ring
		item = newItem(RING_PROTECTION, EXCELLENT, curseItems ? -2 : 2, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// hood silver
		item = newItem(HAT_HOOD_APPRENTICE, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak silver
		item = newItem(CLOAK_SILVER, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// weapon
		item = newItem(MAGICSTAFF_CHARM, EXCELLENT, curseItems ? -1 : 1, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// spear
			item = newItem(MAGICSTAFF_COLD, SERVICABLE, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// confusion
			item = newItem(POTION_CONFUSION, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// cold spellbook
			/*item = newItem(SPELLBOOK_COLD, SERVICABLE, 0, 1, 4, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);*/

			// charm monster spellbook
			item = newItem(SPELLBOOK_CHARM_MONSTER, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_BREWER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// booze
		item = newItem(IRON_AXE, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);
		}

		// empty bottles
		item = newItem(POTION_EMPTY, SERVICABLE, 0, 3, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			equipItem(item2, &stats[player]->weapon, player, false);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			equipItem(item, &stats[player]->weapon, player, false);
		}

		// boots
		item = newItem(IRON_BOOTS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// backpack
		item = newItem(CLOAK_BACKPACK, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			//// weapon
			//item = newItem(IRON_MACE, SERVICABLE, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//hotbar[1].item = item2->uid;
			//free(item);

			// firestorm
			item = newItem(POTION_FIRESTORM, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			// acid
			item = newItem(POTION_ACID, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// booze
			item = newItem(POTION_BOOZE, EXCELLENT, 0, 3, 2, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			// juice
			item = newItem(POTION_JUICE, EXCELLENT, 0, 2, 3, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[7].item = item2->uid;
			free(item);
			
			// polymorph
			item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// blindness
			item = newItem(POTION_BLINDNESS, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// speed
			item = newItem(POTION_SPEED, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// alembic
			item = newItem(TOOL_ALEMBIC, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			item = newItem(READABLE_BOOK, DECREPIT, 0, 1, getBook("Bottle Book"), true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_SHAMAN )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(MAGICSTAFF_POISON, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(MASK_SHAMAN, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		//// ring slow digestion
		//item = newItem(RING_SLOWDIGESTION, SERVICABLE, 0, 1, 0, true, NULL);
		//if ( isLocalPlayer )
		//{
		//	item2 = itemPickup(player, item);
		//	useItem(item2, player);
		//	free(item);
		//}
		//else
		//{
		//	useItem(item, player);
		//}

		if ( isLocalPlayer )
		{
			item = newItem(BRONZE_SWORD, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(GEM_ROCK, WORN, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_PUNISHER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(TOOL_WHIP, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(PUNISHER_HOOD, SERVICABLE, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		//// ring slow digestion
		//item = newItem(RING_SLOWDIGESTION, SERVICABLE, 0, 1, 0, true, NULL);
		//if ( isLocalPlayer )
		//{
		//	item2 = itemPickup(player, item);
		//	useItem(item2, player);
		//	free(item);
		//}
		//else
		//{
		//	useItem(item, player);
		//}

		if ( isLocalPlayer )
		{
			item = newItem(CRYSTAL_BATTLEAXE, DECREPIT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(FOOD_MEAT, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(RING_CONFLICT, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_HUNTER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(LONGBOW, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(AMULET_POISONRESISTANCE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(BRACERS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(LEATHER_BOOTS, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(BOOMERANG, DECREPIT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(QUIVER_SILVER, SERVICABLE, 0, 40, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(QUIVER_LIGHTWEIGHT, SERVICABLE, 0, 40, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			item = newItem(QUIVER_HUNTING, SERVICABLE, 0, 20, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			item = newItem(SCROLL_CONJUREARROW, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_MEAT, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_FISH, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(TOOL_BLINDFOLD_TELEPATHY, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			item = newItem(POTION_SPEED, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			// TO DELETE **********
			//item = newItem(ARTIFACT_BOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(COMPOUND_BOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(SLING, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(SHORTBOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//item = newItem(CROSSBOW, EXCELLENT, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//free(item);

			//for ( int i = QUIVER_SILVER; i <= QUIVER_HUNTING; ++i )
			//{
			//	item = newItem(static_cast<ItemType>(i), EXCELLENT, 1, 48, 0, true, NULL);
			//	item2 = itemPickup(player, item);
			//	free(item);
			//}

			// TO DELETE **********
		}
	}
	else if ( client_classes[player] == CLASS_MACHINIST )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(CROSSBOW, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(TOOL_TINKERING_KIT, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[5].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(MACHINIST_APRON, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(TOOL_BEARTRAP, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(TOOL_DUMMYBOT, DECREPIT, 0, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(TOOL_SENTRYBOT, DECREPIT, 0, 1, ITEM_TINKERING_APPEARANCE, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			item = newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			item = newItem(TOOL_LOCKPICK, EXCELLENT, 0, 4, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			if ( stats[player]->playerRace != RACE_AUTOMATON )
			{
				item = newItem(FOOD_APPLE, EXCELLENT, 0, 2, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);

				item = newItem(FOOD_CHEESE, EXCELLENT, 0, 2, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}
			else
			{
				item = newItem(SCROLL_FIRE, SERVICABLE, 0, 1, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}

			item = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 16, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, 8, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_EMPTY, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_BARD )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false ) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(RAPIER, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(BANDIT_BREASTPIECE, EXCELLENT, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(HAT_PLUMED_CAP, EXCELLENT, 0, 1, 1, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(LOAFERS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(player % 2 == 0 ? INSTRUMENT_FLUTE : INSTRUMENT_LYRE, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(player % 2 == 1 ? INSTRUMENT_FLUTE : INSTRUMENT_LYRE, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_FISH, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_SAPPER )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false ) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(STEEL_FLAIL, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(AMULET_BURNINGRESIST, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(TOOL_FRYING_PAN, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[4].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(SHAWL, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(HOOD_TEAL, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(CLEAT_BOOTS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(GLOVES, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(MASK_EYEPATCH, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(BOLAS, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(TOOL_TOWEL, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			item = newItem(FOOD_MEAT, DECREPIT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_BREAD, DECREPIT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_SCION )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false ) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(TOOL_FOCI_DARK_RIFT, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(MAGICSTAFF_SCEPTER, EXCELLENT, 0, 1, 50, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		/*item = newItem(HAT_CIRCLET_WISDOM, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}*/

		item = newItem(QUILTED_BOOTS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(ROBE_WIZARD, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(FOOD_BREAD, SERVICABLE, 1, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			item = newItem(POTION_RESTOREMAGIC, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			item = newItem(POTION_HEALING, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(KEY_SILVER, WORN, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_HERMIT )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false ) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(SHILLELAGH_MACE, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		Uint32 color = ((uniqueGameKey + player) % MAXPLAYERS);
		item = newItem(TOOL_DUCK, EXCELLENT, 0, 1, MAXPLAYERS * color + player, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(CLOAK_PROTECTION, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(AMULET_WATERBREATHING, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(FOOD_APPLE, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(FOOD_MEAT, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_BOOZE, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(MASK_PIPE, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_PALADIN )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false ) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		item = newItem(CHAIN_COIF, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(CHAIN_HAUBERK, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(CLAYMORE_SWORD, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(TOOL_FOCI_LIGHT_SANCTUARY, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_WATER, EXCELLENT, 4, 4, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	// mod add: whaler
	else if ( client_classes[player] == CLASS_WHALER )
	{
		initClassStats(client_classes[player], stats[player]);

		if (!isLocalPlayer && multiplayer == CLIENT && intro == false) {
			// don't do anything crazy with items on players we don't own
			return;
		}

		// harpoon
		item = newItem(HARPOON, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// bandana
		item = newItem(HAT_BANDANA, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather boots
		item = newItem(LEATHER_BOOTS, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// leather gloves
		item = newItem(GLOVES, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		// cloak
		item = newItem(CLOAK, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}


		item = newItem(AMULET_WATERBREATHING, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			// lantern
			item = newItem(TOOL_LANTERN, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// fish
			item = newItem(FOOD_FISH, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			//booze
			item = newItem(POTION_BOOZE, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(TOOL_BEARTRAP, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

		}
	}
	// mod add: Gunslinger
	else if ( client_classes[player] == CLASS_GUNSLINGER )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false )
		{
			// Do not construct inventory for players this client does not own.
			return;
		}

		item = newItem(FLINTLOCK_PISTOL, DECREPIT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			hotbar[0].item = item2->uid;
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(HAT_BOUNTYHUNTER, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(LEATHER_BOOTS, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(LEATHER_BREASTPIECE, SERVICABLE, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		item = newItem(SPYGLASS, EXCELLENT, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if ( isLocalPlayer )
		{
			item = newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 50, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(SPELLBOOK_ENTRENCH, SERVICABLE, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			item = newItem(SCROLL_MAGICMAPPING, EXCELLENT, 0, 5, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// SERVICABLE is the normal food freshness value displayed as "slightly aged".
			item = newItem(FOOD_MEAT, SERVICABLE, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(BRONZE_SWORD, WORN, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[4].item = item2->uid;
			free(item);

			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_RUNESMITH )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false )
		{
			// Do not construct inventory for players this client does not own.
			return;
		}

		auto equipRunesmithItem = [&](ItemType type, Status status, Sint16 beatitude, Uint32 appearance, int hotbarSlot)
		{
			item = newItem(type, status, beatitude, 1, appearance, true, nullptr);
			if ( isLocalPlayer )
			{
				item2 = itemPickup(player, item);
				useItem(item2, player);
				if ( hotbarSlot >= 0 )
				{
					hotbar[hotbarSlot].item = item2->uid;
				}
				free(item);
			}
			else
			{
				useItem(item, player);
			}
		};

		// Deliberately cursed/binding for normal races; inverted races use +1.
		equipRunesmithItem(RUNE_HAMMER, DECREPIT, curseItems ? 1 : -1, 0, 0);
		equipRunesmithItem(MASK_STEEL_VISOR, WORN, 0, 0, -1);
		equipRunesmithItem(IRON_HELM, DECREPIT, 0, 0, -1);
		equipRunesmithItem(IRON_BREASTPIECE, DECREPIT, 0, 0, -1);
		equipRunesmithItem(IRON_BOOTS, DECREPIT, 0, 0, -1);
		equipRunesmithItem(BRACERS, DECREPIT, 0, 0, -1);
		equipRunesmithItem(CLOAK_PROTECTION, DECREPIT, 0, 0, -1);

		if ( isLocalPlayer )
		{
			item = newItem(SCROLL_IDENTIFY, EXCELLENT, 0, 3, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(GEM_OPAL, EXCELLENT, 0, 1, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// Appearance 8 is the existing convention for DECREPIT starting spellbooks.
			item = newItem(SPELLBOOK_TURN_UNDEAD, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(SPELLBOOK_COWARDICE, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(SPELLBOOK_DISRUPT_EARTH, DECREPIT, 0, 1, 8, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			// Appearance 1 matches the established starting POTION_RESTOREMAGIC convention.
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else if ( client_classes[player] == CLASS_WITCH )
	{
		initClassStats(client_classes[player], stats[player]);

		if ( !isLocalPlayer && multiplayer == CLIENT && intro == false )
		{
			// Do not construct inventory for players this client does not own.
			return;
		}

		auto equipWitchItem = [&](ItemType type, Uint32 appearance)
		{
			item = newItem(type, SERVICABLE, 0, 1, appearance, true, nullptr);
			if ( isLocalPlayer )
			{
				item2 = itemPickup(player, item);
				useItem(item2, player);
				free(item);
			}
			else
			{
				useItem(item, player);
			}
		};

		equipWitchItem(AMULET_FAMILIAR, 0);
		equipWitchItem(ROBE_CULTIST, 0);
		equipWitchItem(HAT_HOOD_APPRENTICE, 1); // hood silver

		if ( isLocalPlayer )
		{
			item = newItem(POTION_EMPTY, SERVICABLE, 0, 10, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_WATER, EXCELLENT, 0, 5, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(OIL_AETHER, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(OIL_MINERAL, SERVICABLE, 0, 2, 0, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}

		if ( multiplayer != CLIENT )
		{
			for ( int category = INGREDIENT_CATEGORY_SLIME;
				category <= INGREDIENT_CATEGORY_ANIMATIC; ++category )
			{
				const auto ingredientCategory =
					static_cast<WitchIngredientCategory>(category);

				// Every Witch contributes eight of each ingredient to vanilla's
				// single shared multiplayer Void Chest.
				Item* contribution = newItem(
					ingredientBagForCategory(ingredientCategory),
					EXCELLENT, 0, 1, 0, true, nullptr);
				if ( contribution )
				{
					for ( int slot = 0; slot < 4; ++slot )
					{
						contribution->ingredientCounts[slot] = 8;
					}
					if ( !mergeIngredientBagIntoVoidChest(*contribution, 0) )
					{
						printlog("[WITCH] ERROR unable to initialize shared starting ingredients");
					}
					free(contribution);
				}
			}
		}
	}
	//mod add end
	stats[player]->OLDHP = stats[player]->HP;

	// mod add: Merrow starting towel
	if ( isLocalPlayer
		&& stats[player]->stat_appearance == 0
		&& stats[player]->playerRace == RACE_MERROW )
	{
		item = newItem(TOOL_TOWEL, DECREPIT, 0, 1, 0, true, nullptr);
		item2 = itemPickup(player, item);
		free(item);
	}
	// mod add end

	if ( stats[player]->stat_appearance == 0 && stats[player]->playerRace == RACE_GOATMAN )
	{
		stats[player]->setEffectActive(EFF_ASLEEP, 1);
		stats[player]->EFFECTS_TIMERS[EFF_ASLEEP] = -1;
		if ( isLocalPlayer )
		{
			// extra booze for hangover :)
			item = newItem(POTION_BOOZE, EXCELLENT, 0, 3, 2, true, nullptr);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else
	{
		stats[player]->clearEffect(EFF_ASLEEP);
		stats[player]->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
	}

	if ( stats[player]->stat_appearance == 0 
		&& (stats[player]->playerRace == RACE_DRYAD || stats[player]->playerRace == RACE_MYCONID)
		&& !stats[player]->helmet )
	{
		stats[player]->setEffectActive(EFF_GROWTH, 3);
		stats[player]->EFFECTS_TIMERS[EFF_GROWTH] = -1;
	}
	else
	{
		stats[player]->clearEffect(EFF_GROWTH);
		stats[player]->EFFECTS_TIMERS[EFF_GROWTH] = 0;
	}

	if ( stats[player]->stat_appearance == 0 && stats[player]->playerRace == RACE_AUTOMATON )
	{
		//stats[player]->HUNGER = 150;
	}

	if ( stats[player]->stat_appearance == 0 
		&& client_classes[player] <= CLASS_MONK 
		&& stats[player]->playerRace != RACE_HUMAN )
	{
		if ( isLocalPlayer )
		{
			// bonus polymorph potions
			if ( stats[player]->playerRace == RACE_DRYAD
				|| stats[player]->playerRace == RACE_MYCONID
				|| stats[player]->playerRace == RACE_GNOME
				|| stats[player]->playerRace == RACE_SALAMANDER )
			{
				/*item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 1, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);*/
			}
			else
			{
				item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 2, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}
		}
	}
	if ( stats[player]->stat_appearance == 0 
		&& ((client_classes[player] >= CLASS_CONJURER 
			&& client_classes[player] <= CLASS_PALADIN)
			|| client_classes[player] == CLASS_WHALER
			|| client_classes[player] == CLASS_GUNSLINGER
			|| client_classes[player] == CLASS_RUNESMITH) // mod add: custom extended classes
		&& stats[player]->playerRace != RACE_HUMAN )
	{
		if ( isLocalPlayer )
		{
			if ( stats[player]->playerRace == RACE_DRYAD
				|| stats[player]->playerRace == RACE_MYCONID
				|| stats[player]->playerRace == RACE_GNOME
				|| stats[player]->playerRace == RACE_SALAMANDER )
			{
				/*item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 1, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);*/
			}
			else
			{
				// bonus polymorph potions
				item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 3, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}
		}
	}
	if ( stats[player]->stat_appearance == 0
		&& client_classes[player] == CLASS_WITCH
		&& stats[player]->playerRace != RACE_HUMAN )
	{
		if ( isLocalPlayer )
		{
			if ( stats[player]->playerRace == RACE_DRYAD
				|| stats[player]->playerRace == RACE_MYCONID
				|| stats[player]->playerRace == RACE_GNOME
				|| stats[player]->playerRace == RACE_SALAMANDER )
			{
				// These races retain the extended-class polymorph-potion exclusion.
			}
			else
			{
				item = newItem(POTION_POLYMORPH, EXCELLENT, 0, 3, 0, true, nullptr);
				item2 = itemPickup(player, item);
				free(item);
			}
		}
	}

	players[player]->mechanics.ducksInARow.clear();
	if ( client_classes[player] == CLASS_HERMIT )
	{
		players[player]->mechanics.ducksInARow.push_back(std::make_pair(((uniqueGameKey + player) % MAXPLAYERS), 0));
	}

	if ( stats[player]->playerRace == RACE_SALAMANDER && stats[player]->stat_appearance == 0 )
	{
		stats[player]->MP = stats[player]->MAXMP / 2;
	}

	/*if ( svFlags & SV_FLAG_LIFESAVING )
	{
		item = newItem(AMULET_LIFESAVING, WORN, 0, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}
	}*/
	if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_BFG) )
	{
		item = newItem(HEAVY_CROSSBOW, EXCELLENT, curseItems ? -99 : 99, 1, 0, true, nullptr);
		if ( isLocalPlayer )
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}
	}
	if ( isLocalPlayer )
	{
		if ( stats[player]->playerRace == RACE_VAMPIRE && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_LEVITATION, player, true);
			addSpell(SPELL_BLEED, player, true);
		}
		else if ( stats[player]->playerRace == RACE_SUCCUBUS && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_TELEPORTATION, player, true);
			addSpell(SPELL_SELF_POLYMORPH, player, true);
		}
		else if ( stats[player]->playerRace == RACE_INSECTOID && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_FLUTTER, player, true);
			addSpell(SPELL_DASH, player, true);
			addSpell(SPELL_ACID_SPRAY, player, true);
		}
		else if ( stats[player]->playerRace == RACE_INCUBUS && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_TELEPORTATION, player, true);
			addSpell(SPELL_SHADOW_TAG, player, true);
		}
		else if ( stats[player]->playerRace == RACE_AUTOMATON && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_SALVAGE, player, true);
		}
		else if ( stats[player]->playerRace == RACE_DRYAD && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_THORNS, player, true);
			addSpell(SPELL_SHRUB, player, true);
		}
		else if ( stats[player]->playerRace == RACE_MYCONID && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_SPORES, player, true);
			addSpell(SPELL_MUSHROOM, player, true);
		}
		else if ( stats[player]->playerRace == RACE_GREMLIN && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_DEFACE, player, true);
		}
		else if ( stats[player]->playerRace == RACE_SALAMANDER && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_BREATHE_FIRE, player, true);
		}
		else if ( stats[player]->playerRace == RACE_GNOME && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_FORGE_JEWEL, player, true);
		}
		else if ( stats[player]->playerRace == RACE_WRAITH && stats[player]->stat_appearance == 0 )
		{
			addSpell(SPELL_POSSESS, player, true);
		}

		if ( stats[player]->getProficiency(PRO_ALCHEMY) >= 0 )
		{
			bool learned = false;
			if ( stats[player]->getProficiency(PRO_ALCHEMY) >= 0 )
			{
				ItemType potion = POTION_WATER;
				learned = GenericGUI[player].alchemyLearnRecipe(potion, false, false);
			}
			if ( stats[player]->getProficiency(PRO_ALCHEMY) >= 20 )
			{
				ItemType potion = POTION_JUICE;
				learned = GenericGUI[player].alchemyLearnRecipe(potion, false, false);
				potion = POTION_BOOZE;
				learned = GenericGUI[player].alchemyLearnRecipe(potion, false, false);
			}
			if ( stats[player]->getProficiency(PRO_ALCHEMY) >= 40 )
			{
				ItemType potion = POTION_ACID;
				learned = GenericGUI[player].alchemyLearnRecipe(potion, false, false);
			}
			if ( stats[player]->getProficiency(PRO_ALCHEMY) >= 60 )
			{
				ItemType potion = POTION_INVISIBILITY;
				learned = GenericGUI[player].alchemyLearnRecipe(potion, false, false);
				potion = POTION_POLYMORPH;
				learned = GenericGUI[player].alchemyLearnRecipe(potion, false, false);
			}
		}

		if ( client_classes[player] == CLASS_SHAMAN )
		{
			addSpell(SPELL_RAT_FORM, player, true);
			addSpell(SPELL_SPIDER_FORM, player, true);
			addSpell(SPELL_TROLL_FORM, player, true);
			addSpell(SPELL_IMP_FORM, player, true);
			addSpell(SPELL_REVERT_FORM, player, true);

			addSpell(SPELL_DETECT_FOOD, player, true);
			addSpell(SPELL_SPEED, player, true);
			addSpell(SPELL_POISON, player, true);
			addSpell(SPELL_SPRAY_WEB, player, true);
			addSpell(SPELL_STRIKE, player, true);
			addSpell(SPELL_FEAR, player, true);
			addSpell(SPELL_LIGHTNING, player, true);
			addSpell(SPELL_CONFUSE, player, true);
			addSpell(SPELL_TROLLS_BLOOD, player, true);
			addSpell(SPELL_AMPLIFY_MAGIC, player, true);
		}
		else if ( client_classes[player] == CLASS_PUNISHER )
		{
			addSpell(SPELL_TELEPULL, player, true);
			addSpell(SPELL_DEMON_ILLUSION, player, true);
		}
		else if ( client_classes[player] == CLASS_CONJURER )
		{
			addSpell(SPELL_SUMMON, player, true);
		}
		else if ( client_classes[player] == CLASS_MESMER )
		{
			addSpell(SPELL_COMMAND, player, true);
		}
		else if ( client_classes[player] == CLASS_BARD )
		{
			addSpell(SPELL_ALTER_INSTRUMENT, player, true);
		}
		else if ( client_classes[player] == CLASS_SAPPER )
		{
			addSpell(SPELL_BOOBY_TRAP, player, true);
		}
		else if ( client_classes[player] == CLASS_ARCANIST )
		{
			addSpell(SPELL_WINDGATE, player, true);
		}
		else if ( client_classes[player] == CLASS_SCION )
		{
			addSpell(SPELL_EARTH_ELEMENTAL, player, true);
			addSpell(SPELL_TELEKINESIS, player, true);
			addSpell(SPELL_BLESS_FOOD, player, true);
		}
		else if ( client_classes[player] == CLASS_HERMIT )
		{
			addSpell(SPELL_MAGICIANS_ARMOR, player, true);
			addSpell(SPELL_PROJECT_SPIRIT, player, true);
			addSpell(SPELL_DEEP_SHADE, player, true);
		}
		else if ( client_classes[player] == CLASS_PALADIN )
		{
			addSpell(SPELL_DIVINE_ZEAL, player, true);
		}
		else if ( client_classes[player] == CLASS_GUNSLINGER ) //mod add: Gunslinger innate utility spells.
		{
			addSpell(SPELL_ENTRENCH, player, true);
			addSpell(SPELL_SALVAGE, player, true);
		}
		else if ( client_classes[player] == CLASS_WITCH )
		{
			addSpell(SPELL_VOID_CHEST, player, true);
		}

		//printlog("spell size: %d", list_Size(&spellList));
		// move default items to the right
		for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
		{
			Item* item = static_cast<Item*>(node->element);
			if ( item )
			{
				if ( items[item->type].item_slot == EQUIPPABLE_IN_SLOT_HELM
					|| items[item->type].item_slot == EQUIPPABLE_IN_SLOT_MASK )
				{
					assert(achievementObserver.playerAchievements->startingClassItems.find(item->type)
						!= achievementObserver.playerAchievements->startingClassItems.end());
				}
				if ( players[player]->paperDoll.enabled && itemIsEquipped(item, player) && item->type != SPELL_ITEM )
				{
					continue;
				}

				//item->x = players[player]->inventoryUI.getSizeX() - item->x - 1;
				if ( item->type == SPELL_ITEM )
				{
					bool skipSpellRearrange = false;
					spell_t* spell = getSpellFromItem(player, item, false);
					if ( spell && client_classes[player] == CLASS_SHAMAN )
					{
						// don't add shapeshift spells to hotbar.
						switch ( spell->ID )
						{
							case SPELL_SPEED:
							case SPELL_POISON:
							case SPELL_SPRAY_WEB:
							case SPELL_STRIKE:
							case SPELL_FEAR:
							case SPELL_LIGHTNING:
							case SPELL_CONFUSE:
							case SPELL_DETECT_FOOD:
							case SPELL_TROLLS_BLOOD:
							case SPELL_AMPLIFY_MAGIC:
								item->appearance += 1000;
								item->y -= 100;
								skipSpellRearrange = true;
								break;
							default:
								break;
						}
					}
					if ( skipSpellRearrange )
					{
						continue;
					}
					for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
					{
						if ( hotbar[i].item == 0 )
						{
							//printlog("%d %s", i, item->getName());
							hotbar[i].item = item->uid;
							break;
						}
					}
				}
			}
		}

		autosortInventory(player);
		players[player]->hud.resetBars();
	}
	//stats[player]->printStats();
	//PlayerCharacterClassManager playerCharacterClassManager(stats[player], client_classes[player]);
	//playerCharacterClassManager.writeToFile();
	//playerCharacterClassManager.readFromFile();
}

void initShapeshiftHotbar(int player)
{
	Uint32 spellRevertUid = 0;
	std::vector<Uint32> monsterSpells;

	if ( stats[player]->type == HUMAN )
	{
		return;
	}

	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();
	auto& hotbar_alternate = hotbar_t.slotsAlternate();

	hotbar_t.swapHotbarOnShapeshift = stats[player]->type;
	auto* newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_DEFAULT]; // the monster's special hotbar.
	spell_t* newSpell = players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT];
	bool shapeshiftHotbarInit = false;
	if ( hotbar_t.swapHotbarOnShapeshift > 0 )
	{
		if ( hotbar_t.swapHotbarOnShapeshift == RAT )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_RAT];
			newSpell = players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_RAT];
			shapeshiftHotbarInit = hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_RAT];
			hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_RAT] = true;
		}
		else if ( hotbar_t.swapHotbarOnShapeshift == SPIDER )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_SPIDER];
			newSpell = players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_SPIDER];
			shapeshiftHotbarInit = hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_SPIDER];
			hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_SPIDER] = true;
		}
		else if ( hotbar_t.swapHotbarOnShapeshift == TROLL )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_TROLL];
			newSpell = players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_TROLL];
			shapeshiftHotbarInit = hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_TROLL];
			hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_TROLL] = true;
		}
		else if ( hotbar_t.swapHotbarOnShapeshift == CREATURE_IMP )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_IMP];
			newSpell = players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_IMP];
			shapeshiftHotbarInit = hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_IMP];
			hotbar_t.hotbarShapeshiftInit[Player::Hotbar_t::HOTBAR_IMP] = true;
		}
	}

	for ( Uint32 slotIndex = 0; slotIndex < NUM_HOTBAR_SLOTS; ++slotIndex )
	{
		hotbar_alternate[Player::Hotbar_t::HOTBAR_DEFAULT][slotIndex].item = hotbar[slotIndex].item; // store our current hotbar.
		hotbar[slotIndex].item = newHotbar->at(slotIndex).item; // load from the monster's hotbar.
	}

	// find "shapeshift" only spells, add em to view.
	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item && item->type == SPELL_ITEM )
		{
			spell_t* spell = getSpellFromItem(player, item, true);
			if ( spell )
			{
				if ( newSpell && newSpell == spell )
				{
					players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT] = players[player]->magic.selectedSpell();
					players[player]->magic.equipSpell(newSpell);
					players[player]->magic.selected_spell_last_appearance = item->appearance;
				}

				if ( spell->ID == SPELL_REVERT_FORM )
				{
					spellRevertUid = item->uid;
					if ( !newSpell )
					{
						players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT] = players[player]->magic.selectedSpell();
						players[player]->magic.equipSpell(spell); // revert form add to spell equipped.
						players[player]->magic.selected_spell_last_appearance = players[player]->magic.selectedSpell()->ID;

					}
				}
				else if ( item->appearance >= 1000 )
				{
					if ( canUseShapeshiftSpellInCurrentForm(player, *item) == 1 )
					{
						monsterSpells.push_back(item->uid);
						item->y += 100;
					}

					if ( item->y >= 0 )
					{
						int y = 0;
						bool notfree = false;
						bool foundaspot = false;
						while ( true )
						{
							for ( int x = 0; x < Player::Inventory_t::MAX_SPELLS_X; x++ )
							{
								for ( node_t* node2 = stats[player]->inventory.first; node2 != nullptr; node2 = node2->next )
								{
									Item* tempItem = static_cast<Item*>(node2->element);
									if ( tempItem == item )
									{
										continue;
									}
									if ( tempItem )
									{
										if ( tempItem->x == x && tempItem->y == y )
										{
											if ( itemCategory(tempItem) == SPELL_CAT )
											{
												notfree = true;  //Both spells. Can't fit in the same slot.
											}
										}
									}
								}
								if ( notfree )
								{
									notfree = false;
									continue;
								}
								item->x = x;
								item->y = y;
								foundaspot = true;
								break;
							}
							if ( foundaspot )
							{
								break;
							}
							y++;
						}
					}
					else
					{
						for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
						{
							if ( hotbar[i].item == item->uid )
							{
								hotbar[i].item = 0;
								hotbar[i].resetLastItem();
							}
						}
					}
				}
			}
		}
	}

	for ( Uint32 i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		if ( hotbar[i].item == 0 )
		{
			continue;
		}
		if ( hotbar[i].item == spellRevertUid )
		{
			spellRevertUid = 0;
		}
		for ( auto& monsterSpell : monsterSpells )
		{
			if ( monsterSpell == hotbar[i].item )
			{
				monsterSpell = 0;
			}
		}
	}

	int i = 0;
	for ( auto it = monsterSpells.begin(); it != monsterSpells.end() && !shapeshiftHotbarInit; ++it )
	{
		if ( *it != 0 )
		{
			hotbar[i].item = *it;
			++i;
		}
	}
	if ( spellRevertUid && !shapeshiftHotbarInit )
	{
		hotbar[4].item = spellRevertUid; // place revert form.
	}
}

void deinitShapeshiftHotbar(int player)
{
	Uint32 swapItem = 0;

	auto& hotbar_t = players[player]->hotbar;
	auto& hotbar = hotbar_t.slots();
	auto& hotbar_alternate = hotbar_t.slotsAlternate();

	auto* newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_DEFAULT]; // the monster's special hotbar.
	spell_t** newSpell = &players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT];
	if ( hotbar_t.swapHotbarOnShapeshift > 0 )
	{
		if ( hotbar_t.swapHotbarOnShapeshift == RAT )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_RAT];
			newSpell = &players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_RAT];
		}
		else if ( hotbar_t.swapHotbarOnShapeshift == SPIDER )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_SPIDER];
			newSpell = &players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_SPIDER];
		}
		else if ( hotbar_t.swapHotbarOnShapeshift == TROLL )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_TROLL];
			newSpell = &players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_TROLL];
		}
		else if ( hotbar_t.swapHotbarOnShapeshift == CREATURE_IMP )
		{
			newHotbar = &hotbar_alternate[Player::Hotbar_t::HOTBAR_IMP];
			newSpell = &players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_IMP];
		}
	}
	for ( Uint32 slotIndex = 0; slotIndex < NUM_HOTBAR_SLOTS; ++slotIndex )
	{
		swapItem = hotbar[slotIndex].item;
		hotbar[slotIndex].item = hotbar_alternate[Player::Hotbar_t::HOTBAR_DEFAULT][slotIndex].item; // swap back to default loadout
		newHotbar->at(slotIndex).item = swapItem;

		// double check for shapeshift spells and remove them.
		Item* item = uidToItem(hotbar[slotIndex].item);
		if ( item && itemCategory(item) == SPELL_CAT && item->appearance >= 1000 )
		{
			if ( canUseShapeshiftSpellInCurrentForm(player, *item) != 1 ) // not allowed to use spell.
			{
				hotbar[slotIndex].item = 0;
				hotbar[slotIndex].resetLastItem();
				hotbar_alternate[Player::Hotbar_t::HOTBAR_DEFAULT][slotIndex].item = 0;
			}
		}
	}
	hotbar_t.swapHotbarOnShapeshift = 0;
	*newSpell = players[player]->magic.selectedSpell();
	players[player]->magic.equipSpell(players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT]);
	if ( players[player]->magic.selectedSpell() )
	{
		players[player]->magic.selected_spell_last_appearance = players[player]->magic.selectedSpell()->ID;
	}
	else
	{
		players[player]->magic.selected_spell_last_appearance = -1;
	}

	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item )
		{
			if ( item->type == SPELL_ITEM && item->appearance >= 1000 )
			{
				spell_t* spell = getSpellFromItem(player, item, true);
				if ( spell && client_classes[player] == CLASS_SHAMAN )
				{
					// move shapeshift spells out of inventory. 
					// if somehow the spell got added to your selected spell then remove it.
					switch ( spell->ID )
					{
						case SPELL_SPEED:
						case SPELL_POISON:
						case SPELL_SPRAY_WEB:
						case SPELL_STRIKE:
						case SPELL_FEAR:
						case SPELL_LIGHTNING:
						case SPELL_CONFUSE:
						case SPELL_DETECT_FOOD:
						case SPELL_TROLLS_BLOOD:
						case SPELL_AMPLIFY_MAGIC:
							if ( item->y >= 0 )
							{
								item->y -= 100;
							}
							if ( players[player]->magic.selectedSpell() == spell )
							{
								players[player]->magic.equipSpell(nullptr);
								players[player]->magic.selected_spell_last_appearance = -1;
							}
							if ( players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT] == spell )
							{
								players[player]->magic.selected_spell_alternate[Player::Hotbar_t::HOTBAR_DEFAULT] = nullptr;
							}
							break;
						default:
							break;
					}
				}
			}
		}
	}
}

bool playerUnlockedShamanSpell(const int player, Item* const item)
{
	if ( player < 0 && player >= MAXPLAYERS )
	{
		return false;
	}

	if ( !stats[player] || !item || item->type != SPELL_ITEM )
	{
		return false;
	}

	spell_t* spell = getSpellFromItem(player, item, false);
	int levelRequirement = 0;
	if ( spell && client_classes[player] == CLASS_SHAMAN )
	{
		if ( item->appearance >= 1000 )
		{
			switch ( spell->ID )
			{
				case SPELL_DETECT_FOOD:
					levelRequirement = 0;
					break;
				case SPELL_SPRAY_WEB:
				case SPELL_SPEED:
					levelRequirement = 3;
					break;
				case SPELL_POISON:
				case SPELL_STRIKE:
					levelRequirement = 6;
					break;
				case SPELL_TROLLS_BLOOD:
				case SPELL_LIGHTNING:
				case SPELL_CONFUSE:
					levelRequirement = 12;
					break;
				case SPELL_FEAR:
				case SPELL_AMPLIFY_MAGIC:
					levelRequirement = 15;
					break;
				default:
					return true;
			}
		}
		else
		{
			switch ( spell->ID )
			{
				case SPELL_RAT_FORM:
					levelRequirement = 0;
					break;
				case SPELL_SPIDER_FORM:
					levelRequirement = 3;
					break;
				case SPELL_TROLL_FORM:
					levelRequirement = 6;
					break;
				case SPELL_IMP_FORM:
					levelRequirement = 12;
					break;
				default:
					return true;
			}
		}
	}

	if ( stats[player]->LVL >= levelRequirement )
	{
		return true;
	}
	return false;
}
