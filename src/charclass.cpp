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
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
using json = nlohmann::json;

/*-------------------------------------------------------------------------------

	initClass

	Sets up a character class for the given player

-------------------------------------------------------------------------------*/
std::map<int,json> classesData;
bool parsed = false;

void parseJSON(){
	std::ifstream jsonFile("classes.json");
	json classJson = json::parse(jsonFile);
	jsonFile.close();
	json classDat;
	for(int i =0; i < classJson["Classes"].size();i++){
		classDat = classJson["Classes"][i];
		classesData[classDat["ID"].get<int>()] = classDat;
	}
	parsed = true;
}




void initClass(int player)
{
	Item *item, *item2;

	if (player == clientnum)
	{
		//TODO: Dedicated gameStartStuff() function. Seriously.
		//(same for deathStuff() and/or gameEndStuff().
		selected_inventory_slot_x = 0;
		selected_inventory_slot_y = 0;
		current_hotbar = 0;

		for (int i = 0; i < NUM_HOTBAR_SLOTS; ++i)
		{
			hotbar[i].item = 0;
		}
	}

	// SEX MODIFIER
	// female; else male
	if (stats[player]->sex)
	{
		stats[player]->DEX += 1;
	}
	else
	{
		stats[player]->STR += 1;
	}

	stats[player]->type = HUMAN;

	//spellList = malloc(sizeof(list_t));
	spellList.first = NULL;
	spellList.last = NULL;

	if(!parsed) parseJSON();

	if(classesData[client_classes[player]] != nullptr){
		json classData = classesData[client_classes[player]];
			stats[player]->STR += classData["Strength"].get<int>();
			stats[player]->DEX += classData["Constitution"].get<int>();
			stats[player]->CON += classData["Dexterity"].get<int>();
			stats[player]->INT += classData["Intelligence"].get<int>();
			stats[player]->PER += classData["Perception"].get<int>();
			stats[player]->CHR += classData["Charisma"].get<int>();
			stats[player]->MAXMP += classData["MaxMP"].get<int>();
			stats[player]->MP += classData["MaxMP"].get<int>();
			stats[player]->MAXHP += classData["MaxHP"].get<int>();
			stats[player]->HP += classData["MaxHP"].get<int>();

			if(classData.contains("Gold"))
				stats[player]->GOLD += classData["Gold"].get<int>();


			json skills = classData["Skills"];

			//Using strings here to make the data more human readable instead of ID:Stat
			if (skills.contains("Lockpicking"))
				stats[player]->PROFICIENCIES[PRO_LOCKPICKING] = skills["Lockpicking"].get<int>();
			if (skills.contains("Stealth"))
				stats[player]->PROFICIENCIES[PRO_STEALTH] = skills["Stealth"].get<int>();
			if (skills.contains("Trading"))
				stats[player]->PROFICIENCIES[PRO_TRADING] = skills["Trading"].get<int>();
			if (skills.contains("Appraisal"))
				stats[player]->PROFICIENCIES[PRO_APPRAISAL] = skills["Appraisal"].get<int>();
			if (skills.contains("Swimming"))
				stats[player]->PROFICIENCIES[PRO_SWIMMING] = skills["Swimming"].get<int>();
			if (skills.contains("Leadership"))
				stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = skills["Leadership"].get<int>();
			if (skills.contains("Spellcasting"))
				stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = skills["Spellcasting"].get<int>();
			if (skills.contains("Magic"))
				stats[player]->PROFICIENCIES[PRO_MAGIC] = skills["Magic"].get<int>();
			if (skills.contains("Ranged"))
				stats[player]->PROFICIENCIES[PRO_RANGED] = skills["Ranged"].get<int>();
			if (skills.contains("Sword"))
				stats[player]->PROFICIENCIES[PRO_SWORD] = skills["Sword"].get<int>();
			if (skills.contains("Mace"))
				stats[player]->PROFICIENCIES[PRO_MACE] = skills["Mace"].get<int>();
			if (skills.contains("Axe"))
				stats[player]->PROFICIENCIES[PRO_AXE] = skills["Axe"].get<int>();
			if (skills.contains("Polearm"))
				stats[player]->PROFICIENCIES[PRO_POLEARM] = skills["Polearm"].get<int>();
			if (skills.contains("Shield"))
				stats[player]->PROFICIENCIES[PRO_SHIELD] = skills["Shield"].get<int>();
			if (skills.contains("Unarmed"))
				stats[player]->PROFICIENCIES[PRO_UNARMED] = skills["Unarmed"].get<int>();
			if (skills.contains("Alchemy"))
				stats[player]->PROFICIENCIES[PRO_ALCHEMY] = skills["Alchemy"].get<int>();

			json items = classData["Items"];
			for (int j = 0; j < items.size(); j++)
			{
				ItemType id = static_cast<ItemType>(items[j]["ID"].get<int>());
				Status status = static_cast<Status>(items[j]["Status"].get<int>());
				int beat = items[j]["Beatitude"].get<int>();
				int count = items[j]["Count"].get<int>();
				int app = items[j]["Appearence"].get<int>();
				bool ident = items[j]["Identified"].get<bool>();
				bool equipped = items[j]["Equipped"].get<bool>();
				item = newItem(id, status, beat, count, app, ident, NULL);
				if (player == clientnum)
				{
					item2 = itemPickup(player, item);
					if (equipped)
					{
						useItem(item2, player);
					}

					if (items[j].contains("Hotbar"))
					{
						hotbar[items[j]["Hotbar"].get<int>()].item = item2->uid;
					}
					free(item);
				}
				else
				{
					if (equipped)
					{
						useItem(item, player);
					}
				}
			}
			json spells = classData["Spells"];
			for(int j = 0; j <spells.size();j++){
				addSpell(spells[j].get<int>(), player, true);
			}
	}

	if (client_classes[player] == CLASS_CONJURER)
	{
		// attributes
		stats[player]->INT += 1;
		stats[player]->CON += 2;
		stats[player]->DEX -= 1;
		stats[player]->PER -= 2;

		stats[player]->MAXHP -= 0;
		stats[player]->HP -= 0;
		stats[player]->MAXMP += 15;
		stats[player]->MP += 15;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 40;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 20;
		stats[player]->PROFICIENCIES[PRO_RANGED] = 20;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 40;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 20;

		// weapon
		item = newItem(MAGICSTAFF_LIGHTNING, EXCELLENT, 0, 1, 0, true, NULL);
		if (player == clientnum)
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

		item = newItem(TOOL_LANTERN, EXCELLENT, 0, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(HAT_HOOD_RED, SERVICABLE, 0, 1, 1, true, NULL);
		if (player == clientnum)
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
		item = newItem(CLOAK, SERVICABLE, 0, 1, 2, true, NULL);
		if (player == clientnum)
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if (player == clientnum)
		{
			// summon book
			item = newItem(SPELLBOOK_SUMMON, DECREPIT, 0, 1, 1, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// slow book
			item = newItem(SPELLBOOK_SLOW, WORN, 0, 1, 8, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 1, 1, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);
		}
	}
	else if (client_classes[player] == CLASS_ACCURSED)
	{
		// attributes
		stats[player]->INT += 10;
		stats[player]->STR -= 2;
		stats[player]->CON -= 2;
		stats[player]->DEX -= 3;
		stats[player]->PER -= 1;

		stats[player]->MAXHP -= 0;
		stats[player]->HP -= 0;
		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 70;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_STEALTH] = 40;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 20;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 40;

		// doublet
		item = newItem(SILVER_DOUBLET, EXCELLENT, 0, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(SUEDE_GLOVES, SERVICABLE, 0, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(SUEDE_BOOTS, SERVICABLE, 0, 1, 0, true, NULL);
		if (player == clientnum)
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if (player == clientnum)
		{
			// invis book
			item = newItem(SPELLBOOK_INVISIBILITY, WORN, 0, 1, 2, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);

			// blood
			item = newItem(FOOD_BLOOD, EXCELLENT, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);
		}
	}
	else if (client_classes[player] == CLASS_MESMER)
	{
		// attributes
		bool curseItems = false;
		if (stats[player]->playerRace == RACE_SUCCUBUS && stats[player]->appearance == 0)
		{
			curseItems = true;
		}

		stats[player]->STR -= 2;
		stats[player]->CON -= 3;
		stats[player]->INT += 2;
		stats[player]->DEX -= 2;
		stats[player]->PER += 2;
		stats[player]->CHR += 4;

		stats[player]->MAXHP -= 5;
		stats[player]->HP -= 5;
		stats[player]->MAXMP += 10;
		stats[player]->MP += 10;

		// skills
		stats[player]->PROFICIENCIES[PRO_MAGIC] = 60;
		stats[player]->PROFICIENCIES[PRO_SPELLCASTING] = 40;
		stats[player]->PROFICIENCIES[PRO_POLEARM] = 20;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 60;

		// ring
		item = newItem(RING_PROTECTION, EXCELLENT, curseItems ? -2 : 2, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(HAT_HOOD_SILVER, SERVICABLE, 0, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(CLOAK_SILVER, SERVICABLE, 0, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(MAGICSTAFF_CHARM, EXCELLENT, curseItems ? -1 : 1, 1, 0, true, NULL);
		if (player == clientnum)
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

		if (player == clientnum)
		{
			// spear
			item = newItem(IRON_SPEAR, SERVICABLE, curseItems ? -1 : 1, 1, 1, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[1].item = item2->uid;
			free(item);

			// restore magic
			item = newItem(POTION_RESTOREMAGIC, EXCELLENT, 0, 2, 1, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[2].item = item2->uid;
			free(item);

			// confusion
			item = newItem(POTION_CONFUSION, EXCELLENT, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[3].item = item2->uid;
			free(item);

			// cold spellbook
			item = newItem(SPELLBOOK_COLD, SERVICABLE, 0, 1, 4, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// charm monster spellbook
			item = newItem(SPELLBOOK_CHARM_MONSTER, DECREPIT, 0, 1, 8, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}
	else if (client_classes[player] == CLASS_BREWER)
	{
		// attributes
		stats[player]->STR += -2;
		stats[player]->DEX += 1;
		stats[player]->CON -= 2;
		stats[player]->INT -= 2;
		stats[player]->PER += 1;
		stats[player]->CHR += 1;

		stats[player]->MAXHP += 10;
		stats[player]->HP += 10;
		stats[player]->MAXMP -= 10;
		stats[player]->MP -= 10;

		stats[player]->GOLD = 100;

		// skills
		/*stats[player]->PROFICIENCIES[PRO_MACE] = 60;
		stats[player]->PROFICIENCIES[PRO_SHIELD] = 40;*/
		stats[player]->PROFICIENCIES[PRO_AXE] = 10;
		stats[player]->PROFICIENCIES[PRO_UNARMED] = 25;
		stats[player]->PROFICIENCIES[PRO_TRADING] = 10;
		stats[player]->PROFICIENCIES[PRO_APPRAISAL] = 10;
		stats[player]->PROFICIENCIES[PRO_LEADERSHIP] = 25;
		stats[player]->PROFICIENCIES[PRO_ALCHEMY] = 50;

		// booze
		item = newItem(IRON_AXE, EXCELLENT, 0, 1, 0, true, NULL);
		if (player == clientnum)
		{
			item2 = itemPickup(player, item);
			hotbar[0].item = item2->uid;
			free(item);
		}

		// empty bottles
		item = newItem(POTION_EMPTY, SERVICABLE, 0, 3, 0, true, NULL);
		if (player == clientnum)
		{
			item2 = itemPickup(player, item);
			equipItem(item2, &stats[player]->weapon, player);
			hotbar[1].item = item2->uid;
			free(item);
		}
		else
		{
			equipItem(item, &stats[player]->weapon, player);
		}

		// boots
		item = newItem(IRON_BOOTS, WORN, 0, 1, 0, true, NULL);
		if (player == clientnum)
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
		item = newItem(CLOAK_BACKPACK, SERVICABLE, 0, 1, 0, true, NULL);
		if (player == clientnum)
		{
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}
		else
		{
			useItem(item, player);
		}

		if (player == clientnum)
		{
			//// weapon
			//item = newItem(IRON_MACE, SERVICABLE, 0, 1, 0, true, NULL);
			//item2 = itemPickup(player, item);
			//hotbar[1].item = item2->uid;
			//free(item);

			// blindness
			item = newItem(POTION_ACID, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[5].item = item2->uid;
			free(item);

			// booze
			item = newItem(POTION_BOOZE, SERVICABLE, 0, 2, 2, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[6].item = item2->uid;
			free(item);

			// juice
			item = newItem(POTION_JUICE, SERVICABLE, 0, 2, 3, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[7].item = item2->uid;
			free(item);

			// alembic
			item = newItem(TOOL_ALEMBIC, EXCELLENT, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			hotbar[8].item = item2->uid;
			free(item);

			// polymorph
			item = newItem(POTION_POLYMORPH, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// blindness
			item = newItem(POTION_BLINDNESS, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// speed
			item = newItem(POTION_SPEED, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			// bread
			item = newItem(FOOD_BREAD, SERVICABLE, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);

			item = newItem(READABLE_BOOK, DECREPIT, 0, 1, getBook("Bottle Book"), true, NULL);
			item2 = itemPickup(player, item);
			hotbar[9].item = item2->uid;
			free(item);
		}
	}

	stats[player]->OLDHP = stats[player]->HP;

	if (stats[player]->appearance == 0 && stats[player]->playerRace == RACE_GOATMAN)
	{
		stats[player]->EFFECTS[EFF_ASLEEP] = true;
		stats[player]->EFFECTS_TIMERS[EFF_ASLEEP] = -1;
		if (player == clientnum)
		{
			// extra booze for hangover :)
			item = newItem(POTION_BOOZE, SERVICABLE, 0, 1, 2, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	else
	{
		stats[player]->EFFECTS[EFF_ASLEEP] = false;
		stats[player]->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
	}

	if (stats[player]->appearance == 0 && client_classes[player] <= CLASS_MONK && stats[player]->playerRace != RACE_HUMAN)
	{
		if (player == clientnum)
		{
			// bonus polymorph potions
			item = newItem(POTION_POLYMORPH, SERVICABLE, 0, 2, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}
	if (stats[player]->appearance == 0 && client_classes[player] >= CLASS_CONJURER && client_classes[player] <= CLASS_BREWER && stats[player]->playerRace != RACE_HUMAN)
	{
		if (player == clientnum)
		{
			// bonus polymorph potions
			item = newItem(POTION_POLYMORPH, SERVICABLE, 0, 3, 0, true, NULL);
			item2 = itemPickup(player, item);
			free(item);
		}
	}

	if (player == clientnum)
	{
		if (svFlags & SV_FLAG_LIFESAVING)
		{
			item = newItem(AMULET_LIFESAVING, WORN, 0, 1, 0, true, NULL);
			item2 = itemPickup(player, item);
			useItem(item2, player);
			free(item);
		}

		if (stats[player]->playerRace == RACE_VAMPIRE && stats[player]->appearance == 0)
		{
			addSpell(SPELL_LEVITATION, player, true);
			addSpell(SPELL_BLEED, player, true);
		}
		else if (stats[player]->playerRace == RACE_SUCCUBUS && stats[player]->appearance == 0)
		{
			addSpell(SPELL_TELEPORTATION, player, true);
		}

		if (stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 0)
		{
			bool learned = false;
			if (stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 0)
			{
				ItemType potion = POTION_WATER;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
			if (stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 20)
			{
				ItemType potion = POTION_JUICE;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
				potion = POTION_BOOZE;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
			if (stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 40)
			{
				ItemType potion = POTION_ACID;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
			if (stats[player]->PROFICIENCIES[PRO_ALCHEMY] >= 60)
			{
				ItemType potion = POTION_INVISIBILITY;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
				potion = POTION_POLYMORPH;
				learned = GenericGUI.alchemyLearnRecipe(potion, false, false);
			}
		}

		//printlog("spell size: %d", list_Size(&spellList));
		// move default items to the right
		for (node_t *node = stats[player]->inventory.first; node != NULL; node = node->next)
		{
			Item *item = (Item *)node->element;
			if (item)
			{
				item->x = INVENTORY_SIZEX - item->x - 1;
				if (item->type == SPELL_ITEM)
				{
					for (int i = 0; i < NUM_HOTBAR_SLOTS; ++i)
					{
						if (hotbar[i].item == 0)
						{
							//printlog("%d %s", i, item->getName());
							hotbar[i].item = item->uid;
							break;
						}
					}
				}
			}
		}
	}
	//stats[clientnum]->printStats();
}
