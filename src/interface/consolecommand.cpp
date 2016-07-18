/*-------------------------------------------------------------------------------

	BARONY
	File: consolecommand.cpp
	Desc: contains consoleCommand()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <sstream>
#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../book.hpp"
#include "../sound.hpp"
#include "../menu.hpp"
#include "../monster.hpp"
#include "../net.hpp"
#include "../paths.hpp"
#include "interface.hpp"

/*-------------------------------------------------------------------------------

	consoleCommand
	
	Takes a string and executes it as a game command

-------------------------------------------------------------------------------*/

void consoleCommand(char *command_str) {
	node_t *node;
	Entity *entity;
	char name[64];
	int c;
	
	if( !command_str )
		return;

	if( !strncmp(command_str,"/ping",5) ) {
		if( multiplayer != CLIENT ) {
			messagePlayer(clientnum,language[1117],0);
		} else {
			strcpy((char *)net_packet->data,"PING");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
			pingtime = SDL_GetTicks();
		}
	}
	else if (!strncmp(command_str, "/fov", 4)) {
		fov = atoi(&command_str[5]);
		fov = std::min(std::max<Uint32>(40,fov),100u);
	}
	else if (!strncmp(command_str, "/svflags ", 9)) {
		if( multiplayer==CLIENT ) {
			messagePlayer(clientnum,language[275]);
		} else {
			svFlags = atoi(&command_str[9]);
			messagePlayer(clientnum,language[276]);

			if( multiplayer==SERVER ) {
				// update client flags
				strcpy((char *)net_packet->data,"SVFL");
				SDLNet_Write32(svFlags,&net_packet->data[4]);
				net_packet->len = 8;

				int c;
				for( c=1; c<MAXPLAYERS; c++ ) {
					if( client_disconnected[c] )
						continue;
					net_packet->address.host = net_clients[c-1].host;
					net_packet->address.port = net_clients[c-1].port;
					sendPacketSafe(net_sock, -1, net_packet, c-1);
					messagePlayer(c,language[276]);
				}
			}
		}
	}
	else if ( !strncmp(command_str, "/lastname ", 10) ) {
		strcpy(name, command_str+10);
		lastname = (string)name;
		lastname = lastname.substr(0, lastname.size() - 1);
	}
	else if( !strncmp(command_str,"/spawnitem ",11) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		strcpy(name,command_str+11);
		for( c=0; c<NUMITEMS; c++ ) {
			if( strstr(items[c].name_identified, name) ) {
				dropItem(newItem(static_cast<ItemType>(c),EXCELLENT,0,1,rand(),TRUE,&stats[clientnum]->inventory),0);
				break;
			}
		}
		if( c==NUMITEMS ) {
			messagePlayer(clientnum,language[278],name);
		}
	}
	else if( !strncmp(command_str,"/spawncursed ",13) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		strcpy(name,command_str+13);
		for( c=0; c<NUMITEMS; c++ ) {
			if( strstr(items[c].name_identified, name) ) {
				dropItem(newItem(static_cast<ItemType>(c),WORN,-2,1,rand(),FALSE,&stats[clientnum]->inventory),0);
				break;
			}
		}
		if( c==NUMITEMS ) {
			messagePlayer(clientnum,language[278],name);
		}
	}
	else if( !strncmp(command_str,"/kick ",6) ) {
		strcpy(name,command_str+6);
		if( multiplayer==SERVER ) {
			for( c=1; c<MAXPLAYERS; c++ ) {
				if( !client_disconnected[c] && !strncmp(name,stats[c]->name,128) ) {
					client_disconnected[c] = TRUE;
					strcpy((char *)net_packet->data,"KICK");
					net_packet->address.host = net_clients[c-1].host;
					net_packet->address.port = net_clients[c-1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, c-1);
					int i;
					for( i=0; i<MAXPLAYERS; i++ ) {
						messagePlayer(i,language[279],c,stats[c]->name);
					}
					break;
				}
			}
			if( c==MAXPLAYERS ) {
				messagePlayer(clientnum,language[280]);
			}
		} else if( multiplayer==CLIENT ) {
			messagePlayer(clientnum,language[281]);
		} else {
			messagePlayer(clientnum,language[282]);
		}
	}
	else if( !strncmp(command_str,"/spawnbook ",11) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		strcpy(name,command_str+11);
		dropItem(newItem(READABLE_BOOK,EXCELLENT,0,1,getBook(name),TRUE,&stats[clientnum]->inventory),0);
	}
	else if( !strncmp(command_str,"/savemap ",9) ) {
		if( command_str[9]!=0 ) {
			saveMap(command_str+9);
			messagePlayer(clientnum,language[283],command_str+9);
		}
	}
	else if( !strncmp(command_str,"/nextlevel",10) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		if( multiplayer==CLIENT ) {
			messagePlayer(clientnum,language[284]);
		} else {
			messagePlayer(clientnum,language[285]);
			loadnextlevel=TRUE;
		}
	}
	else if( !strncmp(command_str,"/pos",4) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		messagePlayer(clientnum,language[286],(int)camera.x,(int)camera.y,(int)camera.z,camera.ang,camera.vang);
	}
	else if( !strncmp(command_str,"/pathmap",4) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		if( players[clientnum] ) {
			int x = std::min<int>(std::max(0.0,floor(players[clientnum]->x/16)),map.width-1);
			int y = std::min<int>(std::max(0.0,floor(players[clientnum]->y/16)),map.height-1);
			messagePlayer(clientnum,"pathMapGrounded value: %d",pathMapGrounded[y+x*map.height]);
			messagePlayer(clientnum,"pathMapFlying value: %d",pathMapFlying[y+x*map.height]);
		}
	}
	else if( !strncmp(command_str,"/exit",5) ) {
		mainloop=0;
	}
	else if( !strncmp(command_str,"/showfps",8) ) {
		showfps=(showfps==FALSE);
	}
	else if( !strncmp(command_str,"/noclip",7) ) {
		if( multiplayer!=SINGLE ) {
			messagePlayer(clientnum,language[287]);
		} else {
			noclip=(noclip==FALSE);
			if( noclip )
				messagePlayer(clientnum,language[288]);
			else
				messagePlayer(clientnum,language[289]);
		}
	}
	else if( !strncmp(command_str,"/god",4) ) {
		if( multiplayer!=SINGLE ) {
			messagePlayer(clientnum,language[290]);
		} else {
			godmode=(godmode==FALSE);
			if( godmode )
				messagePlayer(clientnum,language[291]);
			else
				messagePlayer(clientnum,language[292]);
		}
	}
	else if( !strncmp(command_str,"/buddha",7) ) {
		if( multiplayer!=SINGLE ) {
			messagePlayer(clientnum,language[293]);
		} else {
			buddhamode=(buddhamode==FALSE);
			if( buddhamode )
				messagePlayer(clientnum,language[294]);
			else
				messagePlayer(clientnum,language[295]);
		}
	}
	else if( !strncmp(command_str,"/friendly",9) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		if( multiplayer==CLIENT ) {
			messagePlayer(clientnum,language[284]);
			return;
		}
		everybodyfriendly=(everybodyfriendly==FALSE);
		if( everybodyfriendly )
			messagePlayer(clientnum,language[296]);
		else
			messagePlayer(clientnum,language[297]);
	}
	else if( !strncmp(command_str,"/dowse",6) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		for( node=map.entities->first; node!=NULL; node=node->next ) {
			entity = (Entity *)node->element;
			if( entity->behavior == &actLadder )
				messagePlayer(clientnum,language[298],(int)(entity->x/16),(int)(entity->y/16));
		}
	}
	else if( !strncmp(command_str,"/thirdperson",12) ) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		if( players[clientnum] != NULL ) {
			players[clientnum]->skill[3]=(players[clientnum]->skill[3]==0);
			if( players[clientnum]->skill[3]==1 )
				messagePlayer(clientnum,"thirdperson ON");
			else
				messagePlayer(clientnum,"thirdperson OFF");
		}
	}
	else if( !strncmp(command_str,"/res ",5) ) {
		xres = atoi(&command_str[5]);
		for( c=0; c<strlen(command_str); c++ ) {
			if( command_str[c] == 'x' ) {
				yres = atoi(&command_str[c+1]);
				break;
			}
		}
	}
	else if( !strncmp(command_str,"/rscale",7) ) {
		rscale = atoi(&command_str[8]);
	}
	else if( !strncmp(command_str,"/smoothlighting",15) ) {
		smoothlighting = (smoothlighting==0);
	}
	else if( !strncmp(command_str,"/fullscreen",11) ) {
		fullscreen = (fullscreen==0);
	}
	else if( !strncmp(command_str,"/shaking",8) ) {
		shaking = (shaking==0);
	}
	else if( !strncmp(command_str,"/bobbing",8) ) {
		bobbing = (bobbing==0);
	}
	else if( !strncmp(command_str,"/sfxvolume",10) ) {
		sfxvolume = atoi(&command_str[11]);
	}
	else if( !strncmp(command_str,"/musvolume",10) ) {
		musvolume = atoi(&command_str[11]);
	}
	else if( !strncmp(command_str,"/bind",5) ) {
		if( strstr(command_str,"IN_FORWARD") ) {
			impulses[IN_FORWARD] = atoi(&command_str[6]);
			printlog("Bound IN_FORWARD: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_LEFT") ) {
			impulses[IN_LEFT] = atoi(&command_str[6]);
			printlog("Bound IN_LEFT: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_BACK") ) {
			impulses[IN_BACK] = atoi(&command_str[6]);
			printlog("Bound IN_BACK: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_RIGHT") ) {
			impulses[IN_RIGHT] = atoi(&command_str[6]);
			printlog("Bound IN_RIGHT: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_TURNL") ) {
			impulses[IN_TURNL] = atoi(&command_str[6]);
			printlog("Bound IN_TURNL: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_TURNR") ) {
			impulses[IN_TURNR] = atoi(&command_str[6]);
			printlog("Bound IN_TURNR: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_UP") ) {
			impulses[IN_UP] = atoi(&command_str[6]);
			printlog("Bound IN_UP: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_DOWN") ) {
			impulses[IN_DOWN] = atoi(&command_str[6]);
			printlog("Bound IN_DOWN: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_CHAT") ) {
			impulses[IN_CHAT] = atoi(&command_str[6]);
			printlog("Bound IN_CHAT: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_COMMAND") ) {
			impulses[IN_COMMAND] = atoi(&command_str[6]);
			printlog("Bound IN_COMMAND: %d\n",atoi(&command_str[6]));
		} else if( strstr(command_str,"IN_STATUS") ) {
			impulses[IN_STATUS] = atoi(&command_str[6]);
			printlog("Bound IN_STATUS: %d\n",atoi(&command_str[6]));
		} else if (strstr(command_str, "IN_SPELL_LIST")) {
			impulses[IN_SPELL_LIST] = atoi(&command_str[6]);
			printlog( "Bound IN_SPELL_LIST: %d\n", atoi(&command_str[6]));
		} else if (strstr(command_str, "IN_CAST_SPELL")) {
			impulses[IN_CAST_SPELL] = atoi(&command_str[6]);
			printlog( "Bound IN_CAST_SPELL: %d\n", atoi(&command_str[6]));
		} else if (strstr(command_str, "IN_DEFEND")) {
			impulses[IN_DEFEND] = atoi(&command_str[6]);
			printlog( "Bound IN_DEFEND: %d\n", atoi(&command_str[6]));
		} else if (strstr(command_str, "IN_ATTACK")) {
			impulses[IN_ATTACK] = atoi(&command_str[6]);
			printlog( "Bound IN_ATTACK: %d\n", atoi(&command_str[6]));
		} else if (strstr(command_str, "IN_USE")) {
			impulses[IN_USE] = atoi(&command_str[6]);
			printlog( "Bound IN_USE: %d\n", atoi(&command_str[6]));
		} else {
			messagePlayer(clientnum,"Invalid binding.");
		}
	}
	else if( !strncmp(command_str,"/mousespeed",11) ) {
		mousespeed = atoi(&command_str[12]);
	}
	else if( !strncmp(command_str,"/reversemouse",13) ) {
		reversemouse = (reversemouse==0);
	}
	else if( !strncmp(command_str,"/smoothmouse",12) ) {
		smoothmouse = (smoothmouse==FALSE);
	}
	else if( !strncmp(command_str,"/mana", 4) ) {
		if( multiplayer == SINGLE ) {
			stats[clientnum]->MP = stats[clientnum]->MAXMP;
		} else {
			messagePlayer(clientnum,language[299]);
		}
	}
	else if( !strncmp(command_str,"/heal", 4) ) {
		if( multiplayer == SINGLE ) {
			stats[clientnum]->HP = stats[clientnum]->MAXHP;
		} else {
			messagePlayer(clientnum,language[299]);
		}
	}
	else if (!strncmp(command_str, "/ip ", 4)) {
		if( command_str[4]!=0 ) {
			strcpy(last_ip, command_str + 4);
			last_ip[strlen(last_ip)-1]=0;
		}
	}
	else if (!strncmp(command_str, "/port ", 6)) {
		if (command_str[6] != 0) {
			strcpy(last_port, command_str + 6);
			last_port[strlen(last_port)-1]=0;
		}
	}
	else if (!strncmp(command_str, "/noblood", 8)) {
		spawn_blood = (spawn_blood==FALSE);
	}
	else if(!strncmp(command_str, "/colorblind", 11)) {
		colorblind = (colorblind==FALSE);
	}
	else if (!strncmp(command_str, "/gamma", 6)) {
		std::stringstream ss;
		ss << command_str + 7;
		ss >> vidgamma;
	}
	else if (!strncmp(command_str, "/capturemouse", 13)) {
		capture_mouse = (capture_mouse==FALSE);
	}
	else if (!strncmp(command_str, "/levelup", 8)) {
		if( multiplayer==SINGLE ) {
			if( players[clientnum] )
				players[clientnum]->getStats()->EXP += 100;
		} else {
			messagePlayer(clientnum,language[299]);
		}
	}
	else if (!strncmp(command_str, "/maxout", 7)) {
		if( multiplayer==SINGLE ) {
			int c;
			for( c=0; c<14; c++ )
				consoleCommand("/levelup");
			consoleCommand("/spawnitem steel breastpiece");
			consoleCommand("/spawnitem steel boots");
			consoleCommand("/spawnitem steel gauntlets");
			consoleCommand("/spawnitem steel helm");
			consoleCommand("/spawnitem cloak of magic");
			consoleCommand("/spawnitem steel shield of magic");
			consoleCommand("/spawnitem steel sword");
			consoleCommand("/spawnitem crossbow");
			consoleCommand("/spawnitem magicstaff of lightning");
			for( c=0; c<NUMPROFICIENCIES; c++ ) {
				stats[clientnum]->PROFICIENCIES[c] = 100;
			}
		} else {
			messagePlayer(clientnum,language[299]);
		}
	}
	else if (!strncmp(command_str, "/hunger", 7)) {
		if( multiplayer==SINGLE ) {
			Stat *tempStats = players[clientnum]->getStats();
			if( tempStats )
				tempStats->HUNGER = std::max(0,tempStats->HUNGER-100);
		} else {
			messagePlayer(clientnum,language[299]);
		}
	}
	else if (!strncmp(command_str, "/testsound ", 11)) {
		int num = 0;
		//snprintf((char *)(command_str + 11), strlen(command_str)-11, "%d", num);
		//printlog( "Number is %d. Original is: \"%s\"\n", num, (char *)(&command_str[11]));
		num = atoi((char *)(command_str + 11));
		playSound(num, 256);
	}
	else if (!strncmp(command_str, "/skipintro", 10)) {
		skipintro = (skipintro==FALSE);
	}
	else if (!strncmp(command_str, "/levelmagic", 11)) {
		if( multiplayer==SINGLE ) {
			int i = 0;
			for (; i < 10; ++i) {
				players[clientnum]->increaseSkill(PRO_MAGIC);
				players[clientnum]->increaseSkill(PRO_SPELLCASTING);
			}
		} else {
			messagePlayer(clientnum,language[299]);
		}
	}
	else if (!strncmp(command_str, "/numentities", 12)) {
		messagePlayer(clientnum,language[300],list_Size(map.entities));
	}
	else if (!strncmp(command_str, "/killmonsters", 13)) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		if( multiplayer==CLIENT ) {
			messagePlayer(clientnum, language[284]);
		} else {
			int c=0;
			node_t *node, *nextnode;
			for( node=map.entities->first; node!=NULL; node=nextnode ) {
				nextnode = node->next;
				Entity *entity = (Entity *)node->element;
				if( entity->behavior == &actMonster ) {
					entity->setHP(0);
					c++;
				}
			}
			messagePlayer(clientnum,language[301],c);
		}
	}
	else if (!strncmp(command_str, "/die", 4)) {
		if( multiplayer!=SINGLE ) {
			messagePlayer(clientnum, language[299]);
		} else {
			players[clientnum]->setHP(0);
		}
	}
	else if (!strncmp(command_str, "/segfault", 9)) {
		int* potato = NULL;
		(*potato) = 322; //Crash the game!
	}
	else if (!strncmp(command_str, "/summon ", 8)) {
		if( !(svFlags&SV_FLAG_CHEATS) ) {
			messagePlayer(clientnum,language[277]);
			return;
		}
		if( multiplayer==CLIENT ) {
			messagePlayer(clientnum, language[284]);
		} else if( players[clientnum] ) {
			strcpy(name, command_str+8);
			int i, creature;
			bool found = FALSE;

			for (i = 1; i < NUMMONSTERS; ++i) { //Start at 1 because 0 is a nothing.
				if (strstr(language[90+i], name)) {
					creature = i;
					found = TRUE;
					break;
				}
			}

			if (found) {
				playSoundEntity(players[clientnum], 153, 64);
					
				//Spawn monster
				Entity *monster = summonMonster(static_cast<Monster>(creature),players[clientnum]->x+32*cos(players[clientnum]->yaw),players[clientnum]->y+32*sin(players[clientnum]->yaw));
				if( monster ) {
					messagePlayer(clientnum, language[302], language[90+creature]);
				} else {
					messagePlayer(clientnum, language[303], language[90+creature]);
				}
			} else {
				messagePlayer(clientnum, language[304], name);
			}
		}
	}
	else if (!strncmp(command_str, "/broadcast", 10)) {
		broadcast=(broadcast==FALSE);
	}
	else if (!strncmp(command_str, "/nohud", 6)) {
		nohud=(nohud==FALSE);
	}
	else if (!strncmp(command_str, "/disablehotbarnewitems", 15)) {
		auto_hotbar_new_items = (auto_hotbar_new_items==FALSE);
	}
	else if (!strncmp(command_str, "/lang ", 6)) {
		command_str[8] = 0;
		loadLanguage(command_str+6);
	}
	else if (!strncmp(command_str, "/reloadlang", 11)) {
		reloadLanguage();
	}
	else if (!strncmp(command_str, "/disablemessages", 15)) {
		disable_messages = TRUE;
	}
	else if (!strncmp(command_str, "/right_click_protect", 19)) {
		right_click_protect = TRUE;
	}
	else {
		messagePlayer(clientnum,language[305],command_str);
	}
}