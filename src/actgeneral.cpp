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
#include "sound.hpp"
#include "entity.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "menu.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "scores.hpp"

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
	if ( keystatus[SDL_SCANCODE_HOME] )
	{
		keystatus[SDL_SCANCODE_HOME] = 0;
		my->sprite++;
		if ( my->sprite >= nummodels )
		{
			my->sprite = 0;
		}
		messagePlayer(clientnum, "test sprite: %d", my->sprite);
	}
	if ( keystatus[SDL_SCANCODE_END] )
	{
		keystatus[SDL_SCANCODE_END] = 0;
		my->sprite += 10;
		if ( my->sprite >= nummodels )
		{
			my->sprite = 0;
		}
		messagePlayer(clientnum, "test sprite: %d", my->sprite);
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

	if ( !LIQUID_INIT )
	{
		LIQUID_INIT = 1;
		LIQUID_TIMER = 60 * (rand() % 20);
		if ( LIQUID_LAVA )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 2, 128);
		}
	}
	LIQUID_TIMER--;
	if ( LIQUID_TIMER <= 0 )
	{
		LIQUID_TIMER = 60 * 20 + 60 * (rand() % 20);
		if ( !LIQUID_LAVA )
		{
			playSoundEntityLocal( my, 135, 32 );
		}
		else
		{
			playSoundEntityLocal( my, 155, 100 );
		}
	}
	if ( LIQUID_LAVA && !LIQUID_LAVANOBUBBLE )
	{
		if ( ticks % 40 == my->getUID() % 40 && rand() % 3 == 0 )
		{
			int c, j = 1 + rand() % 2;
			for ( c = 0; c < j; c++ )
			{
				Entity* entity = spawnGib( my );
				entity->x += rand() % 16 - 8;
				entity->y += rand() % 16 - 8;
				entity->flags[SPRITE] = true;
				entity->sprite = 42;
				entity->fskill[3] = 0.01;
				double vel = (rand() % 10) / 20.f;
				entity->vel_x = vel * cos(entity->yaw);
				entity->vel_y = vel * sin(entity->yaw);
				entity->vel_z = -.15 - (rand() % 15) / 100.f;
				entity->z = 7.5;
			}
		}
	}
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
		this->createWorldUITooltip();
		furnitureInit = 1;
		if ( furnitureType == FURNITURE_TABLE || furnitureType == FURNITURE_BUNKBED || furnitureType == FURNITURE_BED || furnitureType == FURNITURE_PODIUM )
		{
			furnitureHealth = 15 + rand() % 5;
		}
		else
		{
			furnitureHealth = 4 + rand() % 4;
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
					entity->y += -3 + rand() % 6;
					entity->x += -3 + rand() % 6;
					entity->z = -5 + rand() % 10;
					entity->yaw = (rand() % 360) * PI / 180.0;
					entity->pitch = (rand() % 360) * PI / 180.0;
					entity->roll = (rand() % 360) * PI / 180.0;
					entity->vel_x = (rand() % 10 - 5) / 10.0;
					entity->vel_y = (rand() % 10 - 5) / 10.0;
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
				if ( (i == 0 && selectedEntity[0] == this) || (client_selected[i] == this) || (splitscreen && selectedEntity[i] == this) )
				{
					if (inrange[i])
					{
						switch ( furnitureType )
						{
							case FURNITURE_CHAIR:
								messagePlayer(i, language[476]);
								break;
							case FURNITURE_TABLE:
								messagePlayer(i, language[477]);
								break;
							case FURNITURE_BED:
								messagePlayer(i, language[2493]);
								break;
							case FURNITURE_BUNKBED:
								if ( i == 0 || i == 2 )
								{
									messagePlayer(i, language[2494]);
								}
								else
								{
									messagePlayer(i, language[2495]);
								}
								break;
							case FURNITURE_PODIUM:
								messagePlayer(i, language[2496]);
								break;
							default:
								messagePlayer(i, language[477]);
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
				if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
				{
					if (inrange[i])
					{
						messagePlayer(i, language[478 + rand() % 5]);
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

	my->actStalagColumn();
}

void Entity::actStalagColumn()
{

}

void actColumn(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}
	if ( my->flags[BLOCKSIGHT] ) // stop the compiler optimising into a different entity.
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
			pistonCamTimer = rand() % 5 * TICKS_PER_SECOND;
		}
	}
	if ( pistonCamDir == 1 ) // up
	{
		z -= 0.1;
		if ( z < -1.75 )
		{
			z = -1.75;
			pistonCamRotateSpeed *= rand() % 2 == 0 ? -1 : 1;
			pistonCamDir = 2; // top
		}
	}
	else if ( pistonCamDir == 2 ) // top
	{
		if ( pistonCamTimer <= 0 )
		{
			pistonCamDir = 3; // down
			pistonCamRotateSpeed = -0.2;
			pistonCamTimer = rand() % 5 * TICKS_PER_SECOND;
		}
	}
	else if ( pistonCamDir == 3 ) // down
	{
		z += 0.1;
		if ( z > 1.75 )
		{
			z = 1.75;
			pistonCamRotateSpeed *= rand() % 2 == 0 ? -1 : 1;
			pistonCamDir = 0; // down
		}
	}
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
	int i;
	for ( i = 0; i < MAXPLAYERS; i++ )
	{
		if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
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


				size_t foundSignpostCharacter = output.find("#");
				if ( foundSignpostCharacter == 0 )
				{
					output.erase(0, 1);
					output.insert(0, "The sign says:\n \"");
					output += "\"";
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
					for ( int i = 0; i < NUMIMPULSES; ++i )
					{
						if ( impulseStrings[i].compare(impulseStr) == 0 )
						{
							output.erase(output.find(impulseStr), impulseStr.length());
							std::string inputFormatted = "[";
							inputFormatted.append(getInputName(impulses[i])).append("]");
							output.insert(foundInputTag, inputFormatted.c_str());
							break;
						}
					}
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
				messagePlayer(i, buf);
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

int TextSourceScript::textSourceProcessScriptTag(std::string& input, std::string findTag)
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
			int attachTo = textSourceProcessScriptTag(input, "@reattachto=");
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
			if ( input.find("@attachrange=") != std::string::npos )
			{
				int result = textSourceProcessScriptTag(input, "@attachrange=");
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
			int result = textSourceProcessScriptTag(input, "@clrplayer");
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
			int result = textSourceProcessScriptTag(input, "@class=");
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
				}

				if ( !applyToEntities.empty() )
				{
					for ( int c = 1; c < MAXPLAYERS; ++c )
					{
						if ( !players[c]->isLocalPlayer() )
						{
							updateClientInformation(c, false, false, TextSourceScript::CLIENT_UPDATE_CLASS);
						}
					}
				}
			}
		}
		else if ( (*it).find("@clrstats") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@clrstats");
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
			int result = textSourceProcessScriptTag(input, "@hunger=");
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
			int result = textSourceProcessScriptTag(input, "@nextlevel=");
			if ( result != k_ScriptError )
			{
				loadnextlevel = true;
				skipLevelsOnLoad = result;
			}
		}
		else if ( (*it).find("@power=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@power=");
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
			int result = textSourceProcessScriptTag(input, "@unpower=");
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
		else if ( (*it).find("@freezemonsters=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@freezemonsters=");
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
			int result = textSourceProcessScriptTag(input, "@unfreezemonsters=");
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
			int result = textSourceProcessScriptTag(input, "@seteffect=");
			if ( result != k_ScriptError )
			{
				int effect = result & 0xFF;
				int effectEndRange = (result >> 8) & 0xFF;
				int duration = (result >> 16) & 0xFF;
				int durationEndRange = (result >> 24) & 0xFF;
				duration = duration + rand() % (std::max(1, (durationEndRange - duration)));
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
			int result = textSourceProcessScriptTag(input, "@clreffect=");
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
			int result = textSourceProcessScriptTag(input, "@findtarget=");
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
											//my->monsterMoveTime = rand() % 10 + 1;
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
			int result = textSourceProcessScriptTag(input, "@killall=");
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
			int result = textSourceProcessScriptTag(input, "@killenemies=");
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
			int result = textSourceProcessScriptTag(input, "@nodropitems=");
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
			int result = textSourceProcessScriptTag(input, profTag);
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
			int result = textSourceProcessScriptTag(input, profTag);
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
			int result = textSourceProcessScriptTag(input, profTag);
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
			int result = textSourceProcessScriptTag(input, profTag);
			if ( result != k_ScriptError )
			{
				achievementObserver.checkMapScriptsOnVariableSet();
				/*for ( auto it = scriptVariables.begin(); it != scriptVariables.end(); ++it )
				{
					printlog("%s | %d", (*it).first.c_str(), (*it).second);
				}*/
			}
		}
		else if ( (*it).find("@addtochest=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@addtochest=");
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
								chest->addItemToChest(item);
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
					int result = textSourceProcessScriptTag(input, profTag);
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
						statOnlyUpdateNeeded = true;
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
					int result = textSourceProcessScriptTag(input, profTag);
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
					int result = textSourceProcessScriptTag(input, profTag);
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
								stats->PROFICIENCIES[i] = result;
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
					int result = textSourceProcessScriptTag(input, profTag);
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
								stats->PROFICIENCIES[i] += result;
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

			Uint32 color = SDL_MapRGB(mainsurface->format, (textSourceColorRGB >> 16) & 0xFF, (textSourceColorRGB >> 8) & 0xFF,
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
				for ( int i = 0; i < NUMIMPULSES; ++i )
				{
					if ( impulseStrings[i].compare(impulseStr) == 0 )
					{
						output.erase(output.find(impulseStr), impulseStr.length());
						std::string inputFormatted = "[";
						inputFormatted.append(getInputName(impulses[i])).append("]");
						output.insert(foundInputTag, inputFormatted.c_str());
						break;
					}
				}
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
							messagePlayerColor(c, color, buf, stats[c]->name);
						}
						else
						{
							messagePlayerColor(c, color, buf);
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
	if ( !stats[player] || client_disconnected[player] )
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
		net_packet->data[11] = (Sint8)stats[player]->EXP;
		net_packet->data[12] = (Sint8)stats[player]->LVL;
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
			net_packet->data[27 + i] = (Uint8)stats[player]->PROFICIENCIES[i];
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 27 + NUMPROFICIENCIES;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		serverUpdatePlayerLVL();
	}
	else if ( updateType == CLIENT_UPDATE_CLASS )
	{
		strcpy((char*)net_packet->data, "SCRC");
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			net_packet->data[4 + i] = client_classes[i];
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 5 + MAXPLAYERS;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
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
		int result = textSourceProcessScriptTag(script, "@triggerif=");
		if ( result != k_ScriptError )
		{
			textSourceScript.setTriggerType(src.textSourceIsScript, static_cast<ScriptTriggeredBy>(result));
		}
	}

	if ( script.find("@attachto=") != std::string::npos )
	{
		int attachTo = textSourceProcessScriptTag(script, "@attachto=");
		if ( attachTo == k_ScriptError )
		{
			return;
		}
		textSourceScript.setScriptType(src.textSourceIsScript, textSourceScript.SCRIPT_ATTACHED);
		int x1 = static_cast<int>(src.x / 16); // default to just whatever this script is sitting on.
		int x2 = static_cast<int>(src.x / 16);
		int y1 = static_cast<int>(src.y / 16);
		int y2 = static_cast<int>(src.y / 16);
		if ( script.find("@attachrange=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(script, "@attachrange=");
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